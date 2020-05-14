#include "accelerometer_handler.h"

#include "config.h"

#include "magic_wand_model_data.h"


#include "tensorflow/lite/c/common.h"

#include "tensorflow/lite/micro/kernels/micro_ops.h"

#include "tensorflow/lite/micro/micro_error_reporter.h"

#include "tensorflow/lite/micro/micro_interpreter.h"

#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"

#include "tensorflow/lite/schema/schema_generated.h"

#include "tensorflow/lite/version.h"

#include "uLCD_4DGL.h"

#include <cmath>

#include "DA7212.h"


#define bufferLength (32)

#define signalLength (1024)


DA7212 audio;

Serial pc(USBTX, USBRX);

// The gesture index of the prediction

int number =0;

int gesture_index;

int last_state =0;

int first_print =0;

int song =0;

int now_song =0;

int change_mode_show =0;

int change_song =0;

int beats[42];

int shift_times =0;

int get_point =0;

int begin =0;

int16_t waveform[kAudioTxBufferSize];

char serialInBuffer[bufferLength];

uLCD_4DGL uLCD(D1, D0, D2);

InterruptIn button(SW2);

DigitalIn  Switch(SW3);

DigitalOut green_led(LED2);

EventQueue DNNqueue(32 * EVENTS_EVENT_SIZE);

EventQueue playqueue(32 * EVENTS_EVENT_SIZE);

//EventQueue taikoqueue(32 * EVENTS_EVENT_SIZE);

Thread DNNthread(osPriorityNormal,80*1024/*120K stack size*/);

Thread playthread(osPriorityNormal,80*1024/*120K stack size*/);

//Thread taikothread(osPriorityNormal,5*1024/*120K stack size*/);

Timer calculate;

int mode =0;

int push =0;

char list[3][15]={"Little star", "An dui", "Little bee"};

//={"Little star", "An dui", "Little bee"};

int main_page =0;

int change_mode_in =0;

int serialCount =0;

int point =0;

float song_note[42];

float noteLength[42];

char name_buffer;

void loadSignal(void)

{

  green_led = 0;

  int i = 0;

  serialCount = 0;

  audio.spk.pause();
  
  serialCount =0;

  while(i < 42)

  {

    if(pc.readable())

    {

      serialInBuffer[serialCount] = pc.getc();

      serialCount++;

      if(serialCount == 5)

      {

        serialInBuffer[serialCount] = '\0';

        song_note[i] = (float) atof(serialInBuffer);

        serialCount = 0;

        //uLCD.printf('%.3f\n',song_note[i]);

        i++;
      }

    }

  }
  i =0;
  serialCount =0;
  while(i < 42)

  {

    if(pc.readable())

    {

      serialInBuffer[serialCount] = pc.getc();

      serialCount++;

      if(serialCount == 5)

      {

        serialInBuffer[serialCount] = '\0';

        noteLength[i] = (float) atof(serialInBuffer);

        serialCount = 0;

        //uLCD.printf('%.3f\n',song_note[i]);

        i++;
      }

    }

  }
  green_led = 1;

}

void load_name(){

  pc.printf("%d\n",4);

  green_led = 0;

  //serialCount =0;

  char name;

  //int namecount =0;

  int i = 0;
  
  while(i < 15)

  {

    if(pc.readable())
  {

      name_buffer = pc.getc();
      list[0][i]=name_buffer;

      //serialCount++;

      //if(serialCount == 15)

     // {

     //   serialInBuffer[serialCount] = '\0';

     //   song_note[i] = (float) atof(serialInBuffer);

     //   serialCount = 0;

        //uLCD.printf('%.3f\n',song_note[i]);

        i++;
      //}

   }

}
  i =0;
  //serialCount =0;
  while(i < 15)

  {

    if(pc.readable())
  {

      name_buffer = pc.getc();
      list[1][i]=name_buffer;

      //serialCount++;

      //if(serialCount == 15)

     // {

     //   serialInBuffer[serialCount] = '\0';

     //   song_note[i] = (float) atof(serialInBuffer);

     //   serialCount = 0;

        //uLCD.printf('%.3f\n',song_note[i]);

        i++;
      //}

   }

}
  i=0;
 // serialCount =0;
  while(i < 15)

  {

    if(pc.readable())
  {

      name_buffer = pc.getc();
      list[2][i]=name_buffer;

      //serialCount++;

      //if(serialCount == 15)

     // {

     //   serialInBuffer[serialCount] = '\0';

     //   song_note[i] = (float) atof(serialInBuffer);

     //   serialCount = 0;

        //uLCD.printf('%.3f\n',song_note[i]);

        i++;
      //}

   }

}
//serialCount =0;
  green_led = 1;
}

void playNote(float freq[])

{
  //(int16_t) (freq[number])*((1<<16)-1) ;

  
    
    float frequency =  freq[number];
    
    for(int j = 0; (j < kAudioSampleFrequency / kAudioTxBufferSize)&& !push; ++j)

    {
      for (int i = 0; i < kAudioTxBufferSize; i++)

      {

      waveform[i] = (int16_t) (sin((double)i * 2. * M_PI/(double) (kAudioSampleFrequency /( 500*frequency))) * ((1<<16) - 1));

      }
      
      audio.spk.play(waveform, kAudioTxBufferSize);

    }
    // the loop below will play the note for the duration of 1s
    
   // uLCD.printf("%.3f\r\n",freq[number]);  
    
    //uLCD.printf("%.3f\r\n",beats[number]);      
}

void change_mode(){
  
  if(push ==0)
  {
    push=1;
  }
  else 
    push=0;
  
  audio.spk.pause();
  first_print =1;
  main_page =0;

}
// Return the result of the last prediction

int PredictGesture(float* output) {

  // How many times the most recent gesture has been matched in a row

  static int continuous_count = 0;

  // The result of the last prediction

  static int last_predict = -1;


  // Find whichever output has a probability > 0.8 (they sum to 1)

  int this_predict = -1;

  for (int i = 0; i < label_num; i++) {

    if (output[i] > 0.8) this_predict = i;

  }


  // No gesture was detected above the threshold

  if (this_predict == -1) {

    continuous_count = 0;

    last_predict = label_num;

    return label_num;

  }


  if (last_predict == this_predict) {

    continuous_count += 1;

  } else {

    continuous_count = 0;

  }

  last_predict = this_predict;


  // If we haven't yet had enough consecutive matches for this gesture,

  // report a negative result

  if (continuous_count < config.consecutiveInferenceThresholds[this_predict]) {

    return label_num;

  }

  // Otherwise, we've seen a positive result, so clear all our variables

  // and report it

  continuous_count = 0;

  last_predict = -1;


  return this_predict;

}

void DNN(){
  // Create an area of memory to use for input, output, and intermediate arrays.

  // The size of this will depend on the model you're using, and may need to be

  // determined by experimentation.

  constexpr int kTensorArenaSize = 60 * 1024;

  uint8_t tensor_arena[kTensorArenaSize];

  // Whether we should clear the buffer next time we fetch data

  bool should_clear_buffer = false;

  bool got_data = false;

  // Set up logging.

  static tflite::MicroErrorReporter micro_error_reporter;

  tflite::ErrorReporter* error_reporter = &micro_error_reporter;


  // Map the model into a usable data structure. This doesn't involve any

  // copying or parsing, it's a very lightweight operation.

  const tflite::Model* model = tflite::GetModel(g_magic_wand_model_data);

  if (model->version() != TFLITE_SCHEMA_VERSION) {

    error_reporter->Report(

        "Model provided is schema version %d not equal "

        "to supported version %d.",

        model->version(), TFLITE_SCHEMA_VERSION);

    return -1;

  }


  // Pull in only the operation implementations we need.

  // This relies on a complete list of all the ops needed by this graph.

  // An easier approach is to just use the AllOpsResolver, but this will

  // incur some penalty in code space for op implementations that are not

  // needed by this graph.

  static tflite::MicroOpResolver<6> micro_op_resolver;

  micro_op_resolver.AddBuiltin(

      tflite::BuiltinOperator_DEPTHWISE_CONV_2D,

      tflite::ops::micro::Register_DEPTHWISE_CONV_2D());

  micro_op_resolver.AddBuiltin(tflite::BuiltinOperator_MAX_POOL_2D,

                               tflite::ops::micro::Register_MAX_POOL_2D());

  micro_op_resolver.AddBuiltin(tflite::BuiltinOperator_CONV_2D,

                               tflite::ops::micro::Register_CONV_2D());

  micro_op_resolver.AddBuiltin(tflite::BuiltinOperator_FULLY_CONNECTED,

                               tflite::ops::micro::Register_FULLY_CONNECTED());

  micro_op_resolver.AddBuiltin(tflite::BuiltinOperator_SOFTMAX,

                               tflite::ops::micro::Register_SOFTMAX());

  micro_op_resolver.AddBuiltin(tflite::BuiltinOperator_RESHAPE,
                               tflite::ops::micro::Register_RESHAPE(),1);

  // Build an interpreter to run the model with

  static tflite::MicroInterpreter static_interpreter(

      model, micro_op_resolver, tensor_arena, kTensorArenaSize, error_reporter);

  tflite::MicroInterpreter* interpreter = &static_interpreter;


  // Allocate memory from the tensor_arena for the model's tensors

  interpreter->AllocateTensors();


  // Obtain pointer to the model's input tensor

  TfLiteTensor* model_input = interpreter->input(0);

  if ((model_input->dims->size != 4) || (model_input->dims->data[0] != 1) ||

      (model_input->dims->data[1] != config.seq_length) ||

      (model_input->dims->data[2] != kChannelNumber) ||

      (model_input->type != kTfLiteFloat32)) {

    error_reporter->Report("Bad input tensor parameters in model");

    return -1;

  }


  int input_length = model_input->bytes / sizeof(float);


  TfLiteStatus setup_status = SetupAccelerometer(error_reporter);

  if (setup_status != kTfLiteOk) {

    error_reporter->Report("Set up failed\n");

    return -1;

  }


  error_reporter->Report("Set up successful...\n");


  while (true) {


    // Attempt to read new data from the accelerometer

    got_data = ReadAccelerometer(error_reporter, model_input->data.f,

                                 input_length, should_clear_buffer);


    // If there was no new data,

    // don't try to clear the buffer again and wait until next time

    if (!got_data) {

      should_clear_buffer = false;

      continue;

    }


    // Run inference, and report any error

    TfLiteStatus invoke_status = interpreter->Invoke();

    if (invoke_status != kTfLiteOk) {

      error_reporter->Report("Invoke failed on index: %d\n", begin_index);

      continue;

    }


    // Analyze the results to obtain a prediction

    gesture_index = PredictGesture(interpreter->output(0)->data.f);
    
  //t.start(callback(&queue, &EventQueue::dispatch_forever));

 // display.start(LCD_display);


    // Clear the buffer next time we read data

    should_clear_buffer = gesture_index < label_num;

  }

}

/*void taiko_beat(){

  while(begin ==0){    
    if(beats[number]==2){
      if(gesture_index==1){
        point =point +1;
        get_point =1;
      }
      else if(gesture_index==0){
        point = point -1;
        get_point =1;
      }
      else{
        get_point =0;
      }
    }
    
    if(beats[number]==1){
      if(gesture_index==0){
        point =point +1;
        get_point =1;
      }
      else if(gesture_index==1){
        point = point -1;
        get_point =1;
      }
      else{
        get_point =0;
      }
    }
}}
*/
int main(int argc, char* argv[]) {

  green_led =1;

  load_name();

  //list={"Little star", "An dui", "Little bee"};

  playthread.start(callback(&playqueue, &EventQueue::dispatch_forever));

 // taikothread.start(callback(&taikoqueue, &EventQueue::dispatch_forever));

  // Whether we should clear the buffer next time we fetch data

  DNNthread.start(DNN);

  button.rise(&change_mode); 

 // load_name();

//  while(pc.readable()){
//    green_led =0;
//  }

  audio.spk.pause();
  
  while(true){
    if(push){
      audio.spk.pause(); 
      point =0;
      if(change_mode_in ==0)
      {
        if(first_print){
          if(mode==0){
            if(now_song > 1)
              song = now_song -1;
            else 
              song =2;
            uLCD.cls();
            uLCD.printf("backward songs\n\n\n\n\n");
            uLCD.printf("%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",list[song][0],list[song][1],list[song][2],list[song][3],list[song][4],list[song][5],list[song][6],list[song][7],list[song][8],list[song][9],list[song][10],list[song][11],list[song][12],list[song][13],list[song][14]);
  
          }
          
          if(mode==1){
            if(now_song < 2)
              song = now_song +1;
            else 
              song =0;
            uLCD.cls();
            uLCD.printf("forward songs\n\n\n\n\n");
            uLCD.printf("%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",list[song][0],list[song][1],list[song][2],list[song][3],list[song][4],list[song][5],list[song][6],list[song][7],list[song][8],list[song][9],list[song][10],list[song][11],list[song][12],list[song][13],list[song][14]);
  
          }
            
          if(mode==2){
            uLCD.cls();
            uLCD.printf("change songs\n\n\n\n\n");
            uLCD.printf("%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",list[0][0],list[0][1],list[0][2],list[0][3],list[0][4],list[0][5],list[0][6],list[0][7],list[0][8],list[0][9],list[0][10],list[0][11],list[0][12],list[0][13],list[0][14]);
            uLCD.printf("%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",list[1][0],list[1][1],list[1][2],list[1][3],list[1][4],list[1][5],list[1][6],list[1][7],list[1][8],list[1][9],list[1][10],list[1][11],list[1][12],list[1][13],list[1][14]);
            uLCD.printf("%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",list[2][0],list[2][1],list[2][2],list[2][3],list[2][4],list[2][5],list[2][6],list[2][7],list[2][8],list[2][9],list[2][10],list[2][11],list[2][12],list[2][13],list[2][14]);

          }
          if(mode==3){
            uLCD.cls();
            uLCD.printf("Taiko game\n\n\n\n\n");
            uLCD.printf("%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",list[0][0],list[0][1],list[0][2],list[0][3],list[0][4],list[0][5],list[0][6],list[0][7],list[0][8],list[0][9],list[0][10],list[0][11],list[0][12],list[0][13],list[0][14]);
            uLCD.printf("%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",list[1][0],list[1][1],list[1][2],list[1][3],list[1][4],list[1][5],list[1][6],list[1][7],list[1][8],list[1][9],list[1][10],list[1][11],list[1][12],list[1][13],list[1][14]);
            uLCD.printf("%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",list[2][0],list[2][1],list[2][2],list[2][3],list[2][4],list[2][5],list[2][6],list[2][7],list[2][8],list[2][9],list[2][10],list[2][11],list[2][12],list[2][13],list[2][14]);
           
          }            
        first_print=0;  
        }

        if(gesture_index ==0){
          last_state=1;
          if(mode<1)
            mode=3;
          else
          {
            mode=mode-1;
          }
          change_song =0;
        }
        if(gesture_index ==1){
          last_state=1;
          if(mode>2)
            mode=0;
          else
          {
            mode=mode+1;
          }
          change_song =0;
        }
        if(mode==0){
          if(last_state){
          
            if(mode==0){
              if(now_song > 0)
                song = now_song -1;
              else 
                song =2;  
            }
            uLCD.cls();
            last_state=0;
            uLCD.printf("backward songs\n\n\n\n\n");
            uLCD.printf("%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",list[song][0],list[song][1],list[song][2],list[song][3],list[song][4],list[song][5],list[song][6],list[song][7],list[song][8],list[song][9],list[song][10],list[song][11],list[song][12],list[song][13],list[song][14]);
  
            main_page =0;
          }
          
    
        //  uLCD.text_width(4); //4X size text

        //  uLCD.text_height(4);

          uLCD.color(RED);

        //  uLCD.locate(1,2);



        }
        if(mode==1){
          if(last_state){
            if(mode==1){
              if(now_song < 2)
                song = now_song +1;
              else 
                song =0;  
            }
            uLCD.cls();
            last_state=0;
            uLCD.printf("forward songs\n\n\n\n\n");
            uLCD.printf("%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",list[song][0],list[song][1],list[song][2],list[song][3],list[song][4],list[song][5],list[song][6],list[song][7],list[song][8],list[song][9],list[song][10],list[song][11],list[song][12],list[song][13],list[song][14]);
            main_page =0;
          }

        //  uLCD.text_width(4); //4X size text

        //  uLCD.text_height(4);

          uLCD.color(RED);

        //  uLCD.locate(1,2);

        

        }
        if(mode==2){
          if(last_state){
            uLCD.cls();
            last_state=0;
            uLCD.printf("change songs\n\n\n\n\n");
            uLCD.printf("%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",list[0][0],list[0][1],list[0][2],list[0][3],list[0][4],list[0][5],list[0][6],list[0][7],list[0][8],list[0][9],list[0][10],list[0][11],list[0][12],list[0][13],list[0][14]);
            uLCD.printf("%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",list[1][0],list[1][1],list[1][2],list[1][3],list[1][4],list[1][5],list[1][6],list[1][7],list[1][8],list[1][9],list[1][10],list[1][11],list[1][12],list[1][13],list[1][14]);
            uLCD.printf("%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",list[2][0],list[2][1],list[2][2],list[2][3],list[2][4],list[2][5],list[2][6],list[2][7],list[2][8],list[2][9],list[2][10],list[2][11],list[2][12],list[2][13],list[2][14]);
            main_page =0;
          }

        //  uLCD.text_width(4); //4X size text

        //  uLCD.text_height(4);

          uLCD.color(RED);
        //  uLCD.locate(1,2);
        if(Switch == 0){
            change_mode_in =1;
        }
      }
         if(mode==3){
          if(last_state){
            uLCD.cls();
            last_state=0;
            uLCD.printf("Taiko game\n\n\n\n\n");
            uLCD.printf("%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",list[0][0],list[0][1],list[0][2],list[0][3],list[0][4],list[0][5],list[0][6],list[0][7],list[0][8],list[0][9],list[0][10],list[0][11],list[0][12],list[0][13],list[0][14]);
            uLCD.printf("%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",list[1][0],list[1][1],list[1][2],list[1][3],list[1][4],list[1][5],list[1][6],list[1][7],list[1][8],list[1][9],list[1][10],list[1][11],list[1][12],list[1][13],list[1][14]);
            uLCD.printf("%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",list[2][0],list[2][1],list[2][2],list[2][3],list[2][4],list[2][5],list[2][6],list[2][7],list[2][8],list[2][9],list[2][10],list[2][11],list[2][12],list[2][13],list[2][14]);
            main_page =0;
          }

        //  uLCD.text_width(4); //4X size text

        //  uLCD.text_height(4);

          uLCD.color(RED);

        //  uLCD.locate(1,2);
            if(Switch == 0){
              change_mode_in =1;
            }
        

        }   
    }
    if(change_mode_in ==1){
        if(gesture_index ==0){
          change_mode_show =0;
          if(song<1)
            song=2;
          else
          {
            song=song-1;
          }
        }
        if(gesture_index ==1){
          change_mode_show =0;
          if(song>1)
            song=0;
          else
          {
            song=song+1;
          }
        }
        //song=now_song;
        if(change_mode_show ==0){
          uLCD.cls();
          change_mode_show =1;

          
          uLCD.color(RED);  
          if(mode == 2) 
            uLCD.printf("change songs\n\n\n\n\n");
          else 
            uLCD.printf("Taiko\n\n\n\n\n");
          if(song == 0){
            uLCD.color(WHITE);
            uLCD.printf("%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",list[0][0],list[0][1],list[0][2],list[0][3],list[0][4],list[0][5],list[0][6],list[0][7],list[0][8],list[0][9],list[0][10],list[0][11],list[0][12],list[0][13],list[0][14]);

            uLCD.color(RED);
            uLCD.background_color(BLACK);

            uLCD.printf("%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",list[1][0],list[1][1],list[1][2],list[1][3],list[1][4],list[1][5],list[1][6],list[1][7],list[1][8],list[1][9],list[1][10],list[1][11],list[1][12],list[1][13],list[1][14]);
            uLCD.printf("%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",list[2][0],list[2][1],list[2][2],list[2][3],list[2][4],list[2][5],list[2][6],list[2][7],list[2][8],list[2][9],list[2][10],list[2][11],list[2][12],list[2][13],list[2][14]);

          }
          if(song == 1){
            uLCD.color(RED);
            uLCD.background_color(BLACK);            
            uLCD.printf("%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",list[0][0],list[0][1],list[0][2],list[0][3],list[0][4],list[0][5],list[0][6],list[0][7],list[0][8],list[0][9],list[0][10],list[0][11],list[0][12],list[0][13],list[0][14]);
       
            uLCD.color(WHITE);
            uLCD.printf("%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",list[1][0],list[1][1],list[1][2],list[1][3],list[1][4],list[1][5],list[1][6],list[1][7],list[1][8],list[1][9],list[1][10],list[1][11],list[1][12],list[1][13],list[1][14]);

            uLCD.color(RED);
            uLCD.background_color(BLACK);            
            uLCD.printf("%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",list[2][0],list[2][1],list[2][2],list[2][3],list[2][4],list[2][5],list[2][6],list[2][7],list[2][8],list[2][9],list[2][10],list[2][11],list[2][12],list[2][13],list[2][14]);


          }
          if(song == 2){
            uLCD.color(RED);       
            uLCD.printf("%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",list[0][0],list[0][1],list[0][2],list[0][3],list[0][4],list[0][5],list[0][6],list[0][7],list[0][8],list[0][9],list[0][10],list[0][11],list[0][12],list[0][13],list[0][14]);
            uLCD.printf("%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",list[1][0],list[1][1],list[1][2],list[1][3],list[1][4],list[1][5],list[1][6],list[1][7],list[1][8],list[1][9],list[1][10],list[1][11],list[1][12],list[1][13],list[1][14]);

            uLCD.color(WHITE);         
            uLCD.printf("%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",list[2][0],list[2][1],list[2][2],list[2][3],list[2][4],list[2][5],list[2][6],list[2][7],list[2][8],list[2][9],list[2][10],list[2][11],list[2][12],list[2][13],list[2][14]);

          }
        }
    }
  }
    else{
      if(main_page ==0){
        uLCD.color(RED);
        uLCD.background_color(BLACK);

        uLCD.cls();
        if(mode!=3){
          uLCD.printf("Song player\n\n\n\n\nNow Playing:\n\n\n%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",list[song][0],list[song][1],list[song][2],list[song][3],list[song][4],list[song][5],list[song][6],list[song][7],list[song][8],list[song][9],list[song][10],list[song][11],list[song][12],list[song][13],list[song][14]);
        }
        else{
          uLCD.printf("Taiko game\n\n\n\n\nNow Playing:\n\n\n%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",list[song][0],list[song][1],list[song][2],list[song][3],list[song][4],list[song][5],list[song][6],list[song][7],list[song][8],list[song][9],list[song][10],list[song][11],list[song][12],list[song][13],list[song][14]);  
        }
        
        main_page =1;
      }
      
      green_led=1;
   
      if((change_song ==0)||(song!=now_song)){
        number =0;
        if(song ==0)
          pc.printf("%d\r\n",1);
        if(song ==1)
          pc.printf("%d\r\n",2);
        if(song ==2)
          pc.printf("%d\r\n",3);
      now_song = song;           
        loadSignal();
        change_song =1;
        pc.printf("%d\r\n",0);
      }
      if(mode ==3){
        for(int i=0; i< 42; i++){
          if(i>=1){
            if((song_note[i]>(0.66))&&(song_note[i-1]!=song_note[i])){
              beats[i] =2;
            }
            else if((song_note[i]<=(0.66))&&(song_note[i-1]!=song_note[i])){
              beats[i] =1;
            }
            else{
              beats[i] =0;
            }
          }
          else{
            if((song_note[i]>(0.66))){
              beats[i] =2;
            }
            else if((song_note[i]<=(0.66))){
              beats[i] =1;
            }
            else{
              beats[i] =0;
            }      
          }
        }
      
      }           
      
      change_mode_in =0;
      change_mode_show =0;
      
      while(change_song&&!push){
        int x1=50;
        int x2=64;
        int x3=78;
        int y=120;
        int radius=4;
//    uLCD.cls();
        if(mode ==3){
          if(beats[number]==2)
            uLCD.filled_circle(x3, y, radius, RED);
          else if(beats[number]==1)
            uLCD.circle(x3, y, radius, RED);
          else
            uLCD.filled_circle(x3, y, radius, BLACK);  

          if(beats[number+1]==2)
            uLCD.filled_circle(x2, y, radius, RED);
          else if(beats[number+1]==1)
            uLCD.circle(x2, y, radius, RED);
          else
            uLCD.filled_circle(x2, y, radius, BLACK);  

          if(beats[number+2]==2)
            uLCD.filled_circle(x1, y, radius, RED);
          else if(beats[number+2]==1)
            uLCD.circle(x1, y, radius, RED);
          else
            uLCD.filled_circle(x1, y, radius, BLACK);  

        }

        playqueue.call(playNote,song_note);
          get_point =0;
          calculate.start();
        if(mode ==3){          
          while(calculate.read()<4*noteLength[number]){
            //get_point =0;
            //uLCD.locate(1,8);
            uLCD.printf("point=%2D\r",point);
            if(beats[number]==2&&get_point ==0){
              if(gesture_index==1){
                point =point +1;
                get_point =1;
              }
              //else if(gesture_index==0){
              //  point = point -1;
              //  get_point =1;
              //}
              //else{
              //  get_point =0;
              //}
            }
            
            if(beats[number]==1&&get_point ==0){
              if(gesture_index==0){
                point =point +1;
                get_point =1;
              }
              //else if(gesture_index==1){
              //  point = point -1;
              //  get_point =1;
              //}
              //else{
              //  get_point =0;
              //}
            }          
          }
        }
        else        
          wait(4*noteLength[number]);
        if(mode ==3){
          calculate.reset();
          //if(get_point ==0)
          //  point = point -1;
          //uLCD.filled_rectangle(1, 70, 127, 90, BLACK);
          uLCD.filled_circle(x1, y, radius, BLACK);
          uLCD.filled_circle(x2, y, radius, BLACK);
          uLCD.filled_circle(x3, y, radius, BLACK);          
        }
        if(number < 42){
          number = number +1;
        }
        else{
          ;
        }
      }
    }
  }
}
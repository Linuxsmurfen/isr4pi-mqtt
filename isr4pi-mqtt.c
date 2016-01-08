/*
  isr4pi to mqtt
  Stefan Jansson 2016-01-08
  Interrupt count that is published to MQTT on an interval

  isr4pi.c
  D. Thiebaut
  based on isr.c from the WiringPi library, authored by Gordon Henderson
  https://github.com/WiringPi/WiringPi/blob/master/examples/isr.c

  Compile as follows: gcc -o isr4pi-mqtt isr4pi-mqtt.c -lwiringPi -lmosquitto 
  Run as follows:     sudo ./isr4pi-mqtt
*/


// Things to include
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <wiringPi.h>         //GPIO libs
#include <mosquitto.h>				//MQTT libs
#include <syslog.h>   				//Only used for debugging in deamon mode

// Variables to set
char mqttserver[20] = "localhost";		    //MQTT server to connect to
int mqttport = 1883;				              //MQTT port to connect to
char mqtttopic[20] = "/node/mypi2/gpio7";	//MQTT topic to post the value to
int interval = 10;				                //The output interval
volatile int eventCounter = 0;			      //The interrupt counter
char payload[10];                         //The value that will be submitted to mqtt
//debug;
//syslog

// Use GPIO Pin 7, which is Pin 7 for wiringPi library
#define BUTTON_PIN 7

// myInterrupt:  called every time an event occurs
void myInterrupt(void) {
   eventCounter++;
}

// ----------------------MAIN------------------------------------------------
int main(void) {

  // become a daemon
  if (daemon(0, 0) < 0)
    err(1, "daemon");

  // sets up the wiringPi library for the GPIO access
  if (wiringPiSetup () < 0) {
      fprintf (stderr, "Unable to setup wiringPi: %s\n", strerror (errno));
      return 1;
  }

  // set pin 7 to generate an interrupt on high-to-low transitions and attach myInterrupt() to the interrupt
  if ( wiringPiISR (BUTTON_PIN, INT_EDGE_FALLING, &myInterrupt) < 0 ) {
      fprintf (stderr, "Unable to setup ISR: %s\n", strerror (errno));
      return 1;
  }

  // init mqtt
  struct mosquitto *mosq = NULL;
  mosquitto_lib_init();
  mosq = mosquitto_new(NULL, true, NULL);
  if(!mosq){
     fprintf(stderr, "Error: Out of memory.\n");
     return 1;
  }

  // Start mqtt
  mosquitto_loop_start(mosq);
  
  // connect mqtt
  //if(mosquitto_connect_async(mosq, mqttserver, mqttport, 60)){
  if(mosquitto_connect(mosq, mqttserver, mqttport, 60)){
     fprintf(stderr, "Unable to connect.\n");
     return 1;
  }
  
  // Endless loop that checks the interrupt counter each X second and pusblish it to mqtt
  while ( 1 ) {
    //printf( "%d\n", eventCounter );								                                //for debug to stdout
    //syslog (LOG_NOTICE, "Mqtt: %d", eventCounter);						                    //for debug to syslog
    sprintf(payload, "%d", eventCounter);							                              //convert the counter value
    eventCounter = 0;										                                            //reset the interrupt counter
    mosquitto_publish(mosq, NULL, mqtttopic, strlen(payload), payload, 1, false);   //post the value to mqtt
    sleep(interval);   										                                          //sleep for X seconds 
  }

  // close mqtt
  mosquitto_loop_stop(mosq, true);
  mosquitto_disconnect(mosq);
  mosquitto_destroy(mosq);
  mosquitto_lib_cleanup();

  return 0;
}

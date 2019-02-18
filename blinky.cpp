#include <Arduino.h>
#include <avr/io.h>
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include "prioq.h"

struct Msg {
	int distance;
	int safeSpeed;
	int desired;
	int curSpeed;
} msg;

#define PIN_LED_YELLOW1 7
#define PIN_LED_YELLOW2 8
#define PIN_LED_YELLOW3 9
#define PIN_LED_RED 6
#define PIN_BUTTON_BRAKE 3
#define PIN_BUTTON_ACC 2
#define STACK_SIZE	200
#define PIN_PTTM	0
#define d 256
#define d2 512
#define d3 768

const int DEBOUNCE_DELAY = 200;
volatile long lastBreakButtonPush = 0;
volatile long lastAccButtonPush = 0;

xQueueHandle driverSpeedSettingQueue, distanceQueue, toSerialQueue;
int safetyBreakFlag;

void slow() {
	long start = millis();
	if (start - lastBreakButtonPush > DEBOUNCE_DELAY) {
		int slowerFlag = -1;
		BaseType_t xHigherPriorityTaskWoken;
		xHigherPriorityTaskWoken = pdFALSE;
		xQueueSendToBackFromISR(driverSpeedSettingQueue, &slowerFlag, &xHigherPriorityTaskWoken);
		lastBreakButtonPush = start;
	}
}

void acc() {
	long start = millis();
	if (start - lastAccButtonPush > DEBOUNCE_DELAY) {
		int fasterFlag = 1;
		BaseType_t xHigherPriorityTaskWoken;
		xHigherPriorityTaskWoken = pdFALSE;
		xQueueSendToBackFromISR(driverSpeedSettingQueue, &fasterFlag, &xHigherPriorityTaskWoken);
		lastAccButtonPush = start;
	}
}

void safetyBreakLightEngage(void *p) {
	TickType_t xLastWakeTime = xTaskGetTickCount();
	while (1) {
		vTaskDelayUntil(&xLastWakeTime, 500);
		if (safetyBreakFlag) {
			//Serial.println("bb2");
			digitalWrite(PIN_LED_RED, HIGH);
			vTaskDelayUntil(&xLastWakeTime, 1250);
			digitalWrite(PIN_LED_RED, LOW);
		}
	}
}

int distanceToSpeedSetting(int dist) {
	if (dist <= d) {
		return 0;
	} else if (dist <= d2) {
		return 1;
	} else if (dist <= d3) {
		return 2;
	} else {
		return 3;
	}
}

void showCurSpeedLed(int curSpeed) {
	if (curSpeed == 3) {
		digitalWrite(PIN_LED_YELLOW1, HIGH);
		digitalWrite(PIN_LED_YELLOW2, HIGH);
		digitalWrite(PIN_LED_YELLOW3, HIGH);
	}
	if (curSpeed == 2) {
		digitalWrite(PIN_LED_YELLOW1, HIGH);
		digitalWrite(PIN_LED_YELLOW2, HIGH);
		digitalWrite(PIN_LED_YELLOW3, LOW);
	}
	if (curSpeed == 1) {
		digitalWrite(PIN_LED_YELLOW1, HIGH);
		digitalWrite(PIN_LED_YELLOW2, LOW);
		digitalWrite(PIN_LED_YELLOW3, LOW);
	}
	if (curSpeed == 0) {
		digitalWrite(PIN_LED_YELLOW1, LOW);
		digitalWrite(PIN_LED_YELLOW2, LOW);
		digitalWrite(PIN_LED_YELLOW3, LOW);
	}
}

void showSpeed(void *p) {
	TickType_t xLastWakeTime = xTaskGetTickCount();
	int distance = 0;
	int driverSpeedSetting = 0;
	int curSpeed = 0;
	while (1) {
		int speedFlag = 0;
		::safetyBreakFlag = 0;
		xQueueReceive(distanceQueue, &distance, 20);
		xQueueReceive(driverSpeedSettingQueue, &speedFlag, 10);
		if (speedFlag) {
			driverSpeedSetting += speedFlag;
			if (driverSpeedSetting < 0) {
				driverSpeedSetting = 0;
			}
			if (driverSpeedSetting > 3) {
				driverSpeedSetting = 3;
			}
		}
		int safeSpeed = distanceToSpeedSetting(distance);
		curSpeed = min(safeSpeed, driverSpeedSetting);
		if (distanceToSpeedSetting(distance) < driverSpeedSetting) {
			::safetyBreakFlag = 1;
		}
		showCurSpeedLed(curSpeed);
		Msg msg;
		msg.curSpeed = curSpeed;
		msg.desired = driverSpeedSetting;
		msg.distance = distance;
		msg.safeSpeed = safeSpeed;
		xQueueSendToBack(toSerialQueue, (void * ) &msg, 10);
		vTaskDelayUntil(&xLastWakeTime, 300);
	}
}

void updateSerial(void *p) {
	TickType_t xLastWakeTime = xTaskGetTickCount();
	while (1) {
		struct Msg msg_recieved;
		xQueueReceive(toSerialQueue, &(msg_recieved), 20);
		Serial.print("Distance ");
		Serial.print(msg_recieved.distance);
		Serial.print("  Safe Speed ");
		Serial.print(msg_recieved.safeSpeed);
		Serial.print("  Desired ");
		Serial.print(msg_recieved.desired);
		Serial.print("  Current Speed ");
		Serial.println(msg_recieved.curSpeed);
		vTaskDelayUntil(&xLastWakeTime, 300);
	}

}

void updateDistance(void *p) {
	TickType_t xLastWakeTime = xTaskGetTickCount();
	while (1) {
		int val = analogRead(PIN_PTTM);
		xQueueSendToBack(distanceQueue, &val, 10);
		vTaskDelayUntil(&xLastWakeTime, 300);
	}
}

void setup() {
	Serial.begin(115200);

	//init the LED
	pinMode(PIN_LED_YELLOW1, OUTPUT);
	pinMode(PIN_LED_YELLOW2, OUTPUT);
	pinMode(PIN_LED_YELLOW3, OUTPUT);
	pinMode(PIN_LED_RED, OUTPUT);

	//init buttons
	pinMode(PIN_BUTTON_BRAKE, INPUT);
	pinMode(PIN_BUTTON_ACC, INPUT);
	attachInterrupt(digitalPinToInterrupt(PIN_BUTTON_BRAKE), slow, RISING);
	attachInterrupt(digitalPinToInterrupt(PIN_BUTTON_ACC), acc, RISING);

	//init queue
	toSerialQueue = xQueueCreate(10, sizeof(struct Msg));
	distanceQueue = xQueueCreate(10, sizeof(int));
	driverSpeedSettingQueue = xQueueCreate(10, sizeof(int));
}

void loop() {
	xTaskCreate(updateDistance, "UpdateDistance", STACK_SIZE, NULL, 4, NULL);
	xTaskCreate(showSpeed, "ShowSpeed", STACK_SIZE, NULL, 3, NULL);
	xTaskCreate(safetyBreakLightEngage, "SafetyBreakLightEngage", STACK_SIZE,
			NULL, 2, NULL);
	xTaskCreate(updateSerial, "UpdateSerial", STACK_SIZE, NULL, 1, NULL);

	vTaskStartScheduler();
}


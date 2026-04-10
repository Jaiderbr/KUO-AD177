#include <ESP32Servo.h>
Servo myservo;
#define Echo 18
#define Trig 19
#define servoPIN 16

#define L_LINE_TRK 32
#define C_LINE_TRK 35
#define R_LINE_TRK 34

int L_TRK_VALUE;
int C_TRK_VALUE;
int R_TRK_VALUE;
int Black_Line = 500;

#define ENA 3
#define ENA_PIN 25
#define IN1 26
#define IN2 27
#define IN3 14
#define IN4 12
#define ENB 4
#define ENB_PIN 13

#define BASE_STRAIGHT   230
#define BASE_CURVE_SOFT 130
#define BASE_CURVE_HARD  80

float smoothedPID = 0;
#define ALPHA 0.30   // 0 = sin cambio, 1 = sin suavizado

int previousError = 0;
int lastError = 0;


void Stop();
void Forward();
void setSpeed(int LS, int RS);

void setup() {
    Serial.begin(9600);

    myservo.attach(servoPIN, 500, 2500);
    myservo.write(90);

    pinMode(Echo, INPUT);
    pinMode(Trig, OUTPUT);
    pinMode(L_LINE_TRK, INPUT);
    pinMode(C_LINE_TRK, INPUT);
    pinMode(R_LINE_TRK, INPUT);

    pinMode(IN1, OUTPUT);
    pinMode(IN2, OUTPUT);
    pinMode(IN3, OUTPUT);
    pinMode(IN4, OUTPUT);

    ledcSetup(ENA, 5000, 8);
    ledcSetup(ENB, 5000, 8);
    ledcAttachPin(ENA_PIN, ENA);
    ledcAttachPin(ENB_PIN, ENB);

    delay(1000);
}

void loop() {

    L_TRK_VALUE = analogRead(L_LINE_TRK);
    C_TRK_VALUE = analogRead(C_LINE_TRK);
    R_TRK_VALUE = analogRead(R_LINE_TRK);

    bool L = (L_TRK_VALUE < Black_Line);
    bool C = (C_TRK_VALUE < Black_Line);
    bool R = (R_TRK_VALUE < Black_Line);
    int error = 0;

    {
        if (!L && !C && !R) { Forward(); return; }
        if (L && C && R) { Forward(); previousError = 0; smoothedPID = 0;return; }
    }

    {
        if (!L && C && !R)      error = +0;
        else if (!L && C && R)  error = +1;
        else if (!L && !C && R) error = +2;
        else if (L && C && !R)  error = -1;
        else if (L && !C && !R) error = -2;
    }

    lastError = error;

    int D = error - previousError;
    previousError = error;

    float KP, KD;
    int vBase;

    if (error == 0) { KP = 25.0;  KD = 140.0; vBase = BASE_STRAIGHT; }
    else if (abs(error) == 1) { KP = 23.0;  KD = 160.0; vBase = BASE_CURVE_SOFT; }
    else { KP = 220.0; KD = 100.0; vBase = BASE_CURVE_HARD; }

    float rawPID = KP * error + KD * D;

    smoothedPID = ALPHA * rawPID + (1.0 - ALPHA) * smoothedPID;

    int leftSpeed = vBase - (int)smoothedPID;
    int rightSpeed = vBase + (int)smoothedPID;

    leftSpeed = constrain(leftSpeed, -120, 255);
    rightSpeed = constrain(rightSpeed, -120, 255);
    setSpeed(leftSpeed, rightSpeed);

}

void Stop() {
    ledcWrite(ENA, 0);
    ledcWrite(ENB, 0);
}

void Forward() {
    ledcWrite(ENA, BASE_STRAIGHT);
    ledcWrite(ENB, BASE_STRAIGHT);
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
}

void setSpeed(int leftSpeed, int rightSpeed) {
    if (leftSpeed >= 0) {
        ledcWrite(ENA, leftSpeed);
        digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
    }
    else {
        ledcWrite(ENA, -leftSpeed);
        digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
    }

    if (rightSpeed >= 0) {
        ledcWrite(ENB, rightSpeed);
        digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);
    }
    else {
        ledcWrite(ENB, -rightSpeed);
        digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
    }
}
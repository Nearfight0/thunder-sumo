/*
  ============================================================================
  Thunder-sumo — autonomous sumo robot (Arduino Nano)
  ============================================================================
  Strategy:
    WAIT     Idle until the IR start module goes HIGH. While waiting, the LED
             shows whether any opponent sensor sees something.
    OPENING  On start, DIP switches DS3/DS4 choose one of four opening moves
             (left flank, right flank, straight charge, back off).
    DEFENCE  After the opening: if the opponent is dead ahead, hold still for
             about 0.8 s and let it come. If it's to the side, turn towards it
             and switch to attack.
    ATTACK   Charge at the opponent with the front sensor, turn towards the side
             sensors, otherwise spin in the direction it was last seen.
    EDGE     In every state, the two line sensors under the front take priority:
             if one sees the white ring edge, back up and turn away from it.
    STOP     If the IR start module goes LOW again, stop and return to WAIT.

  Pin names: sol = left, sag = right, on = front (Azerbaijani).
  ============================================================================
*/

// ── Motor driver (two PWM inputs per motor) ─────────────────────────────────
#define solpwm1 5
#define solpwm2 6
#define sagpwm1 9
#define sagpwm2 10

// ── Line (ring edge) sensors, analog ────────────────────────────────────────
#define solqtr A3            // left line sensor
#define sagqtr A2            // right line sensor
#define LINE_THRESHOLD 600   // analogRead below this = white edge

// ── Opponent sensors (digital, active LOW) ──────────────────────────────────
#define solsensor   A1       // left side
#define solonsensor 4        // front-left
#define onsensor    3        // front
#define sagonsensor 7        // front-right
#define sagsensor   8        // right side

// ── Misc ────────────────────────────────────────────────────────────────────
#define led_buzzer 13
#define ir 11                // IR start module output

// ── Strategy DIP switches ───────────────────────────────────────────────────
// DS1 / DS2 are on A7 / A6, which are analog-only on the Nano (digitalRead
// doesn't work there), so only DS3 / DS4 are used.
#define DS1 A7
#define DS2 A6
#define DS3 A5
#define DS4 A4

#define LAST_LEFT  7
#define LAST_RIGHT 3

int irr = 0;               // 1 once the start signal has been received
int LastValue = 5;         // which side the opponent was last seen on

void Set_Motor(float Lval, float Rval, int timex);

void setup()
{
  pinMode(solpwm1, OUTPUT);
  pinMode(solpwm2, OUTPUT);
  pinMode(sagpwm1, OUTPUT);
  pinMode(sagpwm2, OUTPUT);

  pinMode(solqtr, INPUT_PULLUP);
  pinMode(sagqtr, INPUT_PULLUP);
  pinMode(solsensor, INPUT_PULLUP);
  pinMode(onsensor, INPUT_PULLUP);
  pinMode(sagsensor, INPUT_PULLUP);
  pinMode(solonsensor, INPUT_PULLUP);
  pinMode(sagonsensor, INPUT_PULLUP);

  pinMode(led_buzzer, OUTPUT);
  pinMode(ir, INPUT);

  pinMode(DS3, INPUT_PULLUP);
  pinMode(DS4, INPUT_PULLUP);

  digitalWrite(led_buzzer, LOW);
  digitalWrite(solpwm1, LOW);
  digitalWrite(solpwm2, LOW);
  digitalWrite(sagpwm1, LOW);
  digitalWrite(sagpwm2, LOW);

  // Start-up beeps (tone() doesn't block, so each one needs a delay to be heard)
  tone(led_buzzer, 200, 350);
  delay(400);
  tone(led_buzzer, 300, 200);
  delay(250);
  tone(led_buzzer, 4000, 500);
  delay(550);
  noTone(led_buzzer);
}


void loop() {

Wait:

  if (digitalRead(solsensor) == LOW || digitalRead(solonsensor) == LOW || digitalRead(onsensor) == LOW ||
      digitalRead(sagonsensor) == LOW || digitalRead(sagsensor) == LOW) {
    digitalWrite(led_buzzer, LOW);
  } else {
    digitalWrite(led_buzzer, HIGH);
  }

  if (digitalRead(ir) == 1) {
    irr = 1;

    // Opening move chosen by DIP switches DS3 / DS4
    if (digitalRead(DS3) == 0 && digitalRead(DS4) == 1) {
      // Left flank: turn left, drive forward, turn back right
      Set_Motor(-100, 100, 120);
      Set_Motor(80, 80, 400);
      Set_Motor(100, -100, 120);
      Set_Motor(0, 0, 20);
      goto Defence;
    } else if (digitalRead(DS3) == 1 && digitalRead(DS4) == 0) {
      // Right flank: turn right, drive forward, turn back left
      Set_Motor(100, -100, 120);
      Set_Motor(80, 80, 400);
      Set_Motor(-100, 100, 120);
      Set_Motor(0, 0, 20);
      goto Defence;
    } else if (digitalRead(DS3) == 0 && digitalRead(DS4) == 0) {
      // Straight charge
      Set_Motor(100, 100, 120);
      Set_Motor(0, 0, 20);
      goto Defence;
    } else {
      // Back off
      Set_Motor(-100, -100, 120);
      Set_Motor(0, 0, 20);
      goto Defence;
    }
  }

  goto Wait;


Start:   // ── ATTACK ─────────────────────────────────────────────────────────

  if (digitalRead(ir) == 0 && irr == 1) {
    Set_Motor(0, 0, 1);
    irr = 0;
    goto Wait;
  }

  if (analogRead(solqtr) > LINE_THRESHOLD && analogRead(sagqtr) > LINE_THRESHOLD) {

    if (digitalRead(onsensor) == LOW) {
      Set_Motor(100, 100, 1);
      digitalWrite(led_buzzer, HIGH);
    } else if (digitalRead(solsensor) == LOW) {
      Set_Motor(-102, 102, 1);
      digitalWrite(led_buzzer, HIGH);
      LastValue = LAST_LEFT;
    } else if (digitalRead(solonsensor) == LOW) {
      Set_Motor(-50, 102, 1);
      digitalWrite(led_buzzer, HIGH);
      LastValue = LAST_LEFT;
    } else if (digitalRead(sagsensor) == LOW) {
      Set_Motor(102, -102, 1);
      digitalWrite(led_buzzer, HIGH);
      LastValue = LAST_RIGHT;
    } else if (digitalRead(sagonsensor) == LOW) {
      Set_Motor(102, -50, 1);
      digitalWrite(led_buzzer, HIGH);
      LastValue = LAST_RIGHT;
    } else {
      // Nothing seen: spin towards where the opponent was last
      digitalWrite(led_buzzer, LOW);
      if (LastValue == LAST_LEFT) {
        Set_Motor(-70, 70, 2);
      } else if (LastValue == LAST_RIGHT) {
        Set_Motor(70, -70, 2);
      }
    }

  } else if (analogRead(solqtr) < LINE_THRESHOLD && analogRead(sagqtr) > LINE_THRESHOLD) {
    // Edge on the left: back up, turn right
    Set_Motor(-102, -102, 150);
    Set_Motor(102, -102, 100);
    digitalWrite(led_buzzer, LOW);
  } else if (analogRead(solqtr) > LINE_THRESHOLD && analogRead(sagqtr) < LINE_THRESHOLD) {
    // Edge on the right: back up, turn left
    Set_Motor(-102, -102, 150);
    Set_Motor(-102, 102, 100);
    digitalWrite(led_buzzer, LOW);
  } else if (analogRead(solqtr) < LINE_THRESHOLD && analogRead(sagqtr) < LINE_THRESHOLD) {
    // Edge straight ahead: back up, turn towards the opponent's last side
    if (LastValue == LAST_RIGHT) {
      Set_Motor(-102, -102, 150);
      Set_Motor(102, -102, 100);
    } else {
      Set_Motor(-102, -102, 150);
      Set_Motor(-102, 102, 100);
    }
    digitalWrite(led_buzzer, LOW);
  }

  goto Start;


Defence:   // ── DEFENCE ─────────────────────────────────────────────────────

  if (digitalRead(ir) == 0 && irr == 1) {
    Set_Motor(0, 0, 1);
    irr = 0;
    goto Wait;
  }

  if (analogRead(solqtr) > LINE_THRESHOLD && analogRead(sagqtr) > LINE_THRESHOLD) {

    if (digitalRead(onsensor) == LOW &&
        (digitalRead(solonsensor) == LOW || digitalRead(sagonsensor) == LOW)) {
      // Opponent close and wide in front: attack
      digitalWrite(led_buzzer, HIGH);
      goto Start;
    } else if (digitalRead(onsensor) == LOW) {
      // Opponent dead ahead: hold position for ~800 ms (401 x 2 ms)
      digitalWrite(led_buzzer, HIGH);
      for (int i = 0; i <= 400; i++) {
        Set_Motor(0, 0, 1);
        if (digitalRead(onsensor) == HIGH) {
          goto Defence;
        }
        delay(1);
      }
      goto Start;
    } else if (digitalRead(solsensor) == LOW) {
      Set_Motor(-102, 102, 1);
      digitalWrite(led_buzzer, HIGH);
      LastValue = LAST_LEFT;
      goto Start;
    } else if (digitalRead(solonsensor) == LOW) {
      Set_Motor(-50, 102, 1);
      digitalWrite(led_buzzer, HIGH);
      LastValue = LAST_LEFT;
    } else if (digitalRead(sagsensor) == LOW) {
      Set_Motor(102, -102, 1);
      digitalWrite(led_buzzer, HIGH);
      LastValue = LAST_RIGHT;
      goto Start;
    } else if (digitalRead(sagonsensor) == LOW) {
      Set_Motor(102, -50, 1);
      digitalWrite(led_buzzer, HIGH);
      LastValue = LAST_RIGHT;
    } else {
      digitalWrite(led_buzzer, LOW);
      if (LastValue == LAST_LEFT) {
        Set_Motor(-70, 70, 2);
      } else if (LastValue == LAST_RIGHT) {
        Set_Motor(70, -70, 2);
      }
    }

  } else if (analogRead(solqtr) < LINE_THRESHOLD && analogRead(sagqtr) > LINE_THRESHOLD) {
    Set_Motor(-102, -102, 150);
    Set_Motor(102, -102, 100);
    digitalWrite(led_buzzer, LOW);
  } else if (analogRead(solqtr) > LINE_THRESHOLD && analogRead(sagqtr) < LINE_THRESHOLD) {
    Set_Motor(-102, -102, 150);
    Set_Motor(-102, 102, 100);
    digitalWrite(led_buzzer, LOW);
  } else if (analogRead(solqtr) < LINE_THRESHOLD && analogRead(sagqtr) < LINE_THRESHOLD) {
    if (LastValue == LAST_RIGHT) {
      Set_Motor(-102, -102, 150);
      Set_Motor(102, -102, 100);
    } else {
      Set_Motor(-102, -102, 150);
      Set_Motor(-102, 102, 100);
    }
    digitalWrite(led_buzzer, LOW);
  }

  goto Defence;
}


/*
  Set_Motor(left, right, ms)
  left / right: speed in percent, -102 … 102 (negative = reverse).
  Scaled by 2.5 to the 0-255 PWM range, then held for `ms` milliseconds.
*/
void Set_Motor(float Lval, float Rval, int timex) {
  int l = constrain((int)(Lval * 2.5), -255, 255);
  int r = constrain((int)(Rval * 2.5), -255, 255);

  if (l >= 0) {
    analogWrite(solpwm1, 0);
    analogWrite(solpwm2, l);
  } else {
    analogWrite(solpwm1, -l);
    analogWrite(solpwm2, 0);
  }

  if (r >= 0) {
    analogWrite(sagpwm2, 0);
    analogWrite(sagpwm1, r);
  } else {
    analogWrite(sagpwm2, -r);
    analogWrite(sagpwm1, 0);
  }
  delay(timex);
}

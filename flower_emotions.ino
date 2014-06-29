#include <rgb_matrix.h>
#include <SPI.h>

// RGB matrix
static const int RgbDataPin = 16;
static const int RgbClkPin = 15;
static const int RgbLatchPin = 8;

// Sensor pins
static const int SoilPin = 0;
static const int MovementPin = 1;
static const int LightPin = 2;
static const int SliderPin = 3;

// Sensor parameters
static const int MaxSoilValue = 1023;
static const double DryThreshold = 0.98;
static const double WetThreshold = 0.97;

static const int MovementThreshold = 250;

static const int NightThreshold = 700;
static const int DayThreshold = 650;

static const int MaxSliderValue = 1023;

// Colors
static const int Red = 0x01;
static const int Green = 0x02;
static const int Orange = 0x03;
static const int Blue = 0x04;
static const int SkyBlue = 0x06;
static const int LightPink = 0x07;

// Eyes
static const int OpenEyesDuration = 800;
static const int CloseEyesDuration = 45;
static const int NBlinkStates = 4;

// Mouth
static const int MouthSmile = 0;
static const int MouthOkay = 1;
static const int MouthSad = 2;

// Globals
rgb_matrix Matrix = rgb_matrix(1, 1, RgbDataPin, RgbClkPin, RgbLatchPin);
int8_t Color = 0;

int State = 0; // 0 - Day
                        // 1 - Dry
                        // 2 - Dry, man
                        // 3 - Wet
                        // 4 - Man
                        // 5 - Bye
                        // 6 - Dry, bye
                        // 7 - Night
                        // 8 - Hello
                        // 9 - Dry, hello
unsigned long LastStateChangeTime = 0;
unsigned long StateDuration = 0;

int LocalState = 0;
unsigned long LastLocalStateChangeTime = 0;
unsigned long LocalStateDuration = 0;

// Gettting sensors info
double GetSoilFract() {
    return (double)analogRead(SoilPin) / MaxSoilValue;
}

bool IsDry() {
    return GetSoilFract() >= DryThreshold;
}

bool IsWet() {
    return GetSoilFract() <= WetThreshold;
}

boolean IsMan() {
    return analogRead(MovementPin) <= MovementThreshold;
}

boolean IsDay() {
    return analogRead(LightPin) <= DayThreshold;
}

boolean IsNight() {
    return analogRead(LightPin) >= NightThreshold;
}

double GetSliderFract() {
    return (double)analogRead(SliderPin) / MaxSliderValue;
}

// Painting
void Point(int16_t x, int16_t y) {
    Matrix.plot(y, x, Color, TOP_LAYER);
}

void Line(int16_t x1, int16_t y1, int16_t x2, int16_t y2) {
    Matrix.plot(y1, x1, y2, x2, ADD_DOT, Color, TOP_LAYER);
}

void Eyes(int blink) {
    // Left eye
    if (blink == 3)
        Line(0, 0, 2, 0);
    if (blink == 0 || blink == 3) {
        Point(0, 1);
        Point(2, 1);
    }
    if (blink != 2)
        Point(1, 2);

    // Right eye
    if (blink == 3)
        Line(5, 0, 7, 0);
    if (blink == 0 || blink == 3) {
        Point(5, 1);
        Point(7, 1);
    }
    if (blink != 2)
        Point(6, 2);
}

void Eyes() {
    Eyes(3);
}

void Blink() {
    if (LocalState == NBlinkStates - 1) {
        if (LocalStateDuration > OpenEyesDuration)
            SetLocalState(0);
    }
    else {
        if (LocalStateDuration > CloseEyesDuration)
            SetLocalState(LocalState + 1);
    }
}

void Mouth(int mouthType) {
    Line(2, 6, 5, 6);
    if (mouthType == MouthSmile) {
        Point(1, 5);
        Point(6, 5);
    }
    else if (mouthType == MouthSad) {
        Point(1, 7);
        Point(6, 7);
    }
}

void Nose() {
    Point(3, 4);
    Point(4, 4);
}

void SmileyNeko(int blink) {
    Eyes(blink);
    Nose();
    Mouth(MouthSmile);
}

void SmileyNeko() {
    SmileyNeko(3);
}

void OkayNeko(int blink) {
    Eyes(blink);
    Nose();
    Mouth(MouthOkay);
}

void OkayNeko() {
    OkayNeko(3);
}

void SadNeko(int blink) {
    Eyes(blink);
    Nose();
    Mouth(MouthSad);
}

void SadNeko() {
    SadNeko(3);
}

void PointPos(int pos) {
    Point(pos % 8, pos / 8);
}

// States

void SetState(int state) {
    State = state;
    LocalState = 0;
    LastLocalStateChangeTime = LastStateChangeTime = millis();
}

void SetLocalState(int localState) {
    LocalState = localState;
    LastLocalStateChangeTime = millis();
}

void Day() {
    Color = SkyBlue;
    SmileyNeko();

    if (IsNight())
        SetState(5);
    else if (IsDry())
        SetState(1);
    else if (IsMan())
        SetState(4);
}

void Dry() {
    Color = Red;
    SadNeko();

    if (IsNight())
        SetState(6);
    else if (IsWet())
        SetState(3);
    else if (IsMan())
        SetState(2);
}

void DryMan() {
    Color = Orange;
    SadNeko(LocalState);
    Blink();

    if (IsNight())
        SetState(6);
    else if (IsWet())
        SetState(3);
    else if (!IsMan())
        SetState(1);
}

void Wet() {
    Color = Green;
    if (StateDuration < 3 * CloseEyesDuration + OpenEyesDuration)
        OkayNeko(LocalState);
    else
        SmileyNeko(LocalState);
    Blink();

    if (IsNight())
        SetState(5);
    else if (StateDuration > 3 * (3 * CloseEyesDuration + OpenEyesDuration))
        SetState(0);

}

void Man() {
    Color = Green;
    SmileyNeko(LocalState);
    Blink();

    if (IsNight())
        SetState(5);
    else if (IsDry())
        SetState(1);
    else if (!IsMan())
        SetState(0);
}

void Bye() {
    Color = Blue;
    SmileyNeko(LocalState);
    Blink();

    if (StateDuration > 3 * (3 * CloseEyesDuration + OpenEyesDuration))
        SetState(7);
}

void DryBye() {
    Color = Orange;
    SadNeko(LocalState);
    Blink();

    if (StateDuration > 3 * (3 * CloseEyesDuration + OpenEyesDuration))
        SetState(7);
}

void Night() {
    if (IsDay()) {
        if (IsDry())
            SetState(9);
        else
            SetState(8);
        LocalState = 3;
    }
}

void Hello() {
    Color = Blue;
    SmileyNeko(LocalState);
    Blink();

    if (StateDuration > 3 * (3 * CloseEyesDuration + OpenEyesDuration))
        SetState(0);
}

void DryHello() {
    Color = Orange;
    SadNeko(LocalState);
    Blink();

    if (StateDuration > 3 * (3 * CloseEyesDuration + OpenEyesDuration))
        SetState(1);
}

void SelectStateAndPaint() {
    switch (State) {
        case 0:
            Day();
            break;
        case 1:
            Dry();
            break;
        case 2:
            DryMan();
            break;
        case 3:
            Wet();
            break;
        case 4:
            Man();
            break;
        case 5:
            Bye();
            break;
        case 6:
            DryBye();
            break;
        case 7:
            Night();
            break;
        case 8:
            Hello();
            break;
        case 9:
            DryHello();
            break;
    }
}

// Main loop
void hook() {
    unsigned long time = millis();
    StateDuration = time - LastStateChangeTime;
    LocalStateDuration = time - LastLocalStateChangeTime;

    Matrix.clear();
    SelectStateAndPaint();

    delayMicroseconds(100);
}

// Initialization
void setup() {
    Serial.begin(9600);
    Color = Green;
}

// Main loop caller
void loop() {
    Matrix.display(hook);
}

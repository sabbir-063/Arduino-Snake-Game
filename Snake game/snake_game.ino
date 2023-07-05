#include <MD_MAX72xx.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);
/*
    This is all stuff relating to the display and input hardware. Yes, this is 
    basically a really tiny game console.
*/
int C_PINS[]={13,10,7};
int D_PINS[]={12,8,5};
int S_PINS[]={11,9,6};
#define DEVICES_PER_ROW        3  /* Number of 8x8s per row. Cols are inferred. */
#define VERT_PIN              A0  /* Thumbstick H pin. */
#define HORZ_PIN              A1  /* Thumbstick V pin. */
#define SEL_PIN                0  /* Thumbstick SEL pin. */
#define JOY_CENTRE_POINT     512  /* Thumbstick centre point. */
#define JOY_DEAD_ZONE        100  /* Thumbstick deadzone, measured in whatevers. */
MD_MAX72XX mx1 = MD_MAX72XX(MD_MAX72XX::FC16_HW,D_PINS[0],C_PINS[0],S_PINS[0],DEVICES_PER_ROW);
MD_MAX72XX mx2 = MD_MAX72XX(MD_MAX72XX::FC16_HW,D_PINS[1],C_PINS[1],S_PINS[1],DEVICES_PER_ROW);
MD_MAX72XX mx3 = MD_MAX72XX(MD_MAX72XX::FC16_HW,D_PINS[2],C_PINS[2],S_PINS[2],DEVICES_PER_ROW);
#define	MAX_PIXEL             ((8*DEVICES_PER_ROW)-1)       /* Max pixel co-ord). */
#define PIXELS                (MAX_PIXEL+1)*(MAX_PIXEL*1)   /* Pixel count. */
/*
    These are variables relating to the game.
*/
int ijk=0, mx=0;
#define MAX_TABLETS          max((PIXELS/50),1) /* Max edibles (11). */
#define MAX_SNAKE_SEGS       (PIXELS/2)  /* Max snake segments (288). */
#define GAME_TICK_INTERVAL   200 /* Game timer interval. */
#define JOY_READ_INTERVAL    5  /* Thumbstick read interval. */
#define TABLET_SCORE           1 /* Point value of an edible. */
#define GROWTH                 1 /* How many dots snake grows when it eats. */
int TABLETS[MAX_TABLETS*2] ={0}; /* Edibles. All values are in pairs. */
int SNAKE[MAX_SNAKE_SEGS*2]={0}; /* Snake segments. All values are in pairs. */
int SEGS=0;int DLEN=0;int MX=0;  /* */
int MY=0;int SCORE=0;            /* Snake control vars. */
String lastscore="";             /* */
/*
    This code is called when the microcontroller resets.
*/
void setup() {
    lcd.init(); // initialize the lcd
    lcd.backlight();
    mx1.begin();mx1.control(MD_MAX72XX::INTENSITY,MAX_INTENSITY/2);mx1.clear();
    mx2.begin();mx2.control(MD_MAX72XX::INTENSITY,MAX_INTENSITY/2);mx2.clear();
    mx3.begin();mx3.control(MD_MAX72XX::INTENSITY,MAX_INTENSITY/2);mx3.clear();
    mx1.control(MD_MAX72XX::INTENSITY,MAX_INTENSITY/2);
    pinMode(VERT_PIN,INPUT);pinMode(HORZ_PIN,INPUT);pinMode(SEL_PIN,INPUT_PULLUP);
    GameReset();
    ijk=1;
}
/*
    Here are clock functions. These control how fast different parts of the game 
    code can runs. Call ClockIncTimeGates() at the start of loop(). Anywhere
    in loop(), you can call ClockTestTimeGate() to see if a particular interval of 
    time has elapsed. The lengths of CTIME and PTIME determine how many time gates 
    can be tested against. SM0L SN3K tests against two time gates.
*/
unsigned long CTIME[8]={0,0}; // space for 2 gates (thumbstick and game)
unsigned long PTIME[8]={0,0};

void ClockReset(){
  for(int i=0;i<8;i++){
    CTIME[i]=millis();PTIME[i]=0;
    }
}

void ClockIncTimeGates(){
  for(int i=0;i<8;i++){
    CTIME[i]=millis();
  } 
}

int ClockTestTimeGate(int gate,int desired_interval){
  if ((CTIME[gate]-PTIME[gate]) >= desired_interval){
    PTIME[gate]=CTIME[gate];return true;
  }
  return false;
}
/*
    Here are some convenience functions for working with four chained dot matrix 
    sections and for handling display wrapping of pixel positions.
*/
void Clear(){mx1.clear();mx2.clear();mx3.clear();}
void Done(){mx1.update();mx2.update();mx3.update();}

int  Clamp(int n){
  if(n<0){
    n=MAX_PIXEL+1+n;
  }
  if(n>MAX_PIXEL){
    n=MAX_PIXEL+1-n;
  }
  return n;
}
void DrawSnake(){
  int i=0;
  while(i<SEGS){
    Plot(SNAKE[i*2+0],SNAKE[i*2+1],true);i++;
  }
}
/*
    This code plots a pixel to one of the bottom three dot matrix rows. Which 
    device is selected depends on the Y co-ordinate. This is quite expandable.
*/
void Plot(int x,int y,int on){
    if((x<0)||(x>MAX_PIXEL)||(y<0)||(y>MAX_PIXEL)){return;} // prevent bad draws

    if(y>15){mx3.setPoint(y-16,MAX_PIXEL-x,on);}
    else if(y>7) {mx2.setPoint(y-8,MAX_PIXEL-x,on);}
    else {mx1.setPoint(y,MAX_PIXEL-x,on);}               // can be expanded
}
/*
    This code plots a pixel to the top dot matrix row. This row is used exclusively 
    for displaying the score and whatever else I can come up with.
*/
/*
    This code plots a numeric character to the top dot matrix row using the helper 
    function PSB() which takes care of pixel addressing.
*/

/*
    This code plots the score or logo to the top dot matrix row.
*/
/*
    This code plots tables on the bottom three dot matrix rows.
*/
void DrawTablets(){
    int i=0;
    int len=sizeof(TABLETS)/sizeof(TABLETS[0])/2;
    while(i<len){
        if((TABLETS[i*2+0]>-1)&&(TABLETS[i*2+1]>-1)){
            Plot(TABLETS[i*2+0],TABLETS[i*2+1],true);
        }
        i++;
    }
}
/*
    This code manages game initialisation and resets.
*/
void GameReset(){
    if(ijk){
      mx = max(mx, SCORE); //lcd.clear();
      // lcd.setCursor(0, 0); lcd.println("Game Over");
      // delay(1000);
      lcd.setCursor(0, 0);  lcd.print("Your Score : ");  
      lcd.setCursor(13, 0); lcd.println(String(SCORE)); 
      lcd.setCursor(0, 1);  lcd.print("Max Score : ");  
      lcd.setCursor(12, 1);  lcd.println(String(mx)); 
      delay(4000);
      lcd.clear();
    }
    lcd.setCursor(0, 0); lcd.print("Snake Game");
    lcd.setCursor(3, 1); lcd.print("Starting...");   
    //PrintText("SM0L");delay(500);PrintText("SN3K");delay(500);
    InitSnake();InitTablets();Clear();SCORE=0;ClockReset();
    delay(3000); lcd.clear();
    lcd.setCursor(0, 0);  lcd.print("Your Score : ");
}
void GameLost(){
    delay(1000);GameReset(); 
}
void GameWon(){
    delay(1000);GameReset();
}
/*
    This code generates MAX_TABLETS tablets for the snake to eat. We try to 
    avoid placing the new tablet either on the snake or on an existing tablet, 
    and there's a sentinel value to prevent an infinite freeze in case where 
    no tablet can be placed.
*/
void InitTablets(){
    randomSeed(analogRead(A3));

    for(int i=0;i<(MAX_TABLETS*2);i++){TABLETS[i]=-1;}

    int tx=0;int ty=0;int tries=0;
    
    for(int i=0;i<MAX_TABLETS;i++){
      tx=random(0,MAX_PIXEL);ty=random(0,MAX_PIXEL);
      tries=0;
      while(CheckTabletOnSnake(tx,ty)||(CheckTabletOnTablet(tx,ty))){
        tx=random(0,MAX_PIXEL);ty=random(0,MAX_PIXEL);
        if(tries>=255){
          GameLost();return;
        } // whoops!
      }
      TABLETS[i*2+0]=tx;TABLETS[i*2+1]=ty;
    }
}
/*
    This code initialises the snake with MAX_SNAKE_SEGS total length and 
    an actual length of SEGS. By adjusting DLEN, we can grow the snake.
*/
void InitSnake(){
    for(int i=0;i<(MAX_SNAKE_SEGS*2);i++){SNAKE[i]=-1;}

    SNAKE[0]=11;SNAKE[1]=11;SEGS=1;
    DLEN=4;
    MY=-1;MX=0;
    lastscore=98760978;
}
/*
    This code checks to see if desired tablet position TX,TY would end 
    up hitting the snake.
*/
int CheckTabletOnSnake(int tx,int ty){
  int i=0;
  while(i<SEGS){
    if((SNAKE[i*2+0]>-1)&&(SNAKE[i*2+1]>-1)){          // ignore NULLs
      if((SNAKE[i*2+0]==tx)&&(SNAKE[i*2+1]==ty)){      // collision?
          return true;                                  // game lose!
      }
    }
    i++;
  }
  return false;
}
/*
    This code checks to see if desired tablet position TX,TY would end 
    up hitting an existing tablet.
*/
int CheckTabletOnTablet(int tx,int ty){
  int i=0;
  while(i<MAX_TABLETS){
    if((TABLETS[i*2+0]>-1)&&(TABLETS[i*2+1]>-1)){      // ignore NULLs
      if((TABLETS[i*2+0]==tx)&&(TABLETS[i*2+1]==ty)){  // collision?
        return true;                                  // game lose!
      }
    }
    i++;
  }
  return false;
}
/*
    This code checks to see if snake self-collision has occurred. If it has, 
    the game calls GameLost() and resets.
*/
void CheckSnakeSelfCollide(){
  int hX=SNAKE[(SEGS-1)*2+0];
  int hY=SNAKE[(SEGS-1)*2+1]; // get snake head position
  int i=0;
  while(i<SEGS-1){
    if((SNAKE[i*2+0]>-1)&&(SNAKE[i*2+1]>-1)){          // ignore NULLs
      if((SNAKE[i*2+0]==hX)&&(SNAKE[i*2+1]==hY)){      // collision?
          GameLost();return;                            // game lose!
      }
    }
    i++;
  }
}
/*
    This code checks to see if a tablet has been eaten by the snake. If one 
    has, a new random tablet is generated. We try to avoid placing the new
    tablet either on the snake or on an existing tablet, and there's a 
    sentinel value to prevent an infinite freeze in case where no tablet 
    can be placed.
*/
void TryEatTablet(){
  int len=sizeof(TABLETS)/sizeof(TABLETS[0])/2;
  int i=0;int tx;int ty;
  int hX=SNAKE[(SEGS-1)*2+0];int hY=SNAKE[(SEGS-1)*2+1]; // get snake head position
  int tries;
  while(i<len){
    if((TABLETS[i*2+0]>-1)&&(TABLETS[i*2+1]>-1)){                  
      if((TABLETS[i*2+0]==hX)&&(TABLETS[i*2+1]==hY)){ 
        Plot(TABLETS[i*2+0],TABLETS[i*2+1],false);
        TABLETS[i*2+0]=-1;TABLETS[i*2+1]=-1;
        tx=random(0,MAX_PIXEL);ty=random(0,MAX_PIXEL);tries=0;

        while(CheckTabletOnSnake(tx,ty)||(CheckTabletOnTablet(tx,ty))){
            tx=random(0,MAX_PIXEL);ty=random(0,MAX_PIXEL);
            if(tries>=255){GameLost();return;} // whoops!
        }

        TABLETS[i*2+0]=tx;TABLETS[i*2+1]=ty;        
        DLEN+=GROWTH;SCORE+=TABLET_SCORE;
      }
    }
    i++;
  }
}
/*
    This code moves the snake, and also handles snake growth. This routine is a 
    pretty big deviation from my 'real' SNAKE games, as the Arduino doesn't have 
    dynamic lists as such that can be easily used.
*/
void MoveSnake(){
    int hX=SNAKE[(SEGS-1)*2+0];int hY=SNAKE[(SEGS-1)*2+1];// get snake head position
    int tX=SNAKE[0];int tY=SNAKE[1]; 
                         // get snake tail position
    if(DLEN>SEGS){SEGS++;}                                // grow snake?
    if(SEGS>MAX_SNAKE_SEGS){
      GameWon();return;
    }            // snake is too big
    Plot(tX,tY,false);                                    // unplot tail pixel
    hX+=MX;hY+=MY;                                        // shift position of head
    int q=0;while(q<SEGS){                                // 
      SNAKE[(q*2)+0]=SNAKE[(q*2)+2];                      //   shift snake segments 
      SNAKE[(q*2)+1]=SNAKE[(q*2)+3];                      //   
      q++;                                                //   
    }                                                     //
    SNAKE[(SEGS-1)*2+0]=Clamp(hX);SNAKE[(SEGS-1)*2+1]=Clamp(hY); // replace head
}
/*
    This code is called every tick. We're using millis() to avoid using delay().
    This gives better thumbstick responsiveness. [2022.05.20]
*/
void loop() {
  ClockIncTimeGates();

  if (ClockTestTimeGate(0, GAME_TICK_INTERVAL)) {
    lcd.setCursor(13, 0);
    lcd.println(String(SCORE));
    MoveSnake();
    TryEatTablet();
    DrawSnake();
    DrawTablets();
    CheckSnakeSelfCollide();
    Done();
  }
  
  if (ClockTestTimeGate(1, JOY_READ_INTERVAL)) {
    int horz = analogRead(HORZ_PIN);
    int vert = analogRead(VERT_PIN);
   if (horz < (JOY_CENTRE_POINT - JOY_DEAD_ZONE)) {
  MX = -1;  // Move left
  MY = 0;
} else if (horz > (JOY_CENTRE_POINT + JOY_DEAD_ZONE)) {
  MX = 1;   // Move right
  MY = 0;
} else if (vert < (JOY_CENTRE_POINT - JOY_DEAD_ZONE)) {
  MX = 0;
  MY = 1;  // Move up
} else if (vert > (JOY_CENTRE_POINT + JOY_DEAD_ZONE)) {
  MX = 0;
  MY = -1;   // Move down
}

  }
}
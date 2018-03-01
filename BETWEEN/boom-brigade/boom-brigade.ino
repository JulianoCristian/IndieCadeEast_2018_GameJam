/*
   BOOM BRIGADE

   Description:
   2-5 players assume the role of a bomb squad in a tense electronic party game reminiscent of Hot Potato or Russian Roulette.

   Actions:
   Long Press resets all Blinks to get ready

   Double Click on center Blink to start the game

   Press to try and diffuse the bomb,
   Note: it has a higher and higher liklihood of exploding the more clicks it has had this round

   Remove pieces of the shield that have exploded (been completely damaged)

   Players are eliminated if the bomb goes off and there is no shield to protect them

   Game Design by:
   Jeff Kowalski, Holly Gore, Collin Gallo

   Recoded by:
   Jonathan Bobrow
*/

#define SHIELD_MAX_HEALTH  4
#define SHIELD_MIN_HEALTH  0

#define MAX_CLICK_COUNT    10

#define SHOW_FACE_DURATION_MS  750

enum Modes {
  READY,          // waiting for the game to start
  BOMB,           // center piece you are trying to defuse
  SHIELD,         // surrounding pieces to protect you from the bomb
  SPARK           // let the shield know it's been damaged
};

byte mode = READY;

byte shieldHealth;

Timer bombTickTimer;
Timer bombShowFaceTimer;

bool  bSpinning;
bool  bExplode;
byte  bombTickFace;
byte  bombClickCount;

void setup() {

  // Initialize all of our variables
  resetToReady();
}

void loop() {

  /*
     Button Actions
  */
  if ( buttonPressed() ) {

    if ( mode == BOMB ) {

      // we were just pushed, give user feedback on which face they just landed on
      bombShowFaceTimer.set( SHOW_FACE_DURATION_MS );

      if (bombClickCount < MAX_CLICK_COUNT) {

        bombClickCount++;

        // chance of explode based on clickCount
        if ( rand(100) < bombClickCount * 5 ) {
          bExplode = true;
          bSpinning = false;
        }
      }
      else {
        // we've been clicked the maximum amount of times
        // time to explode
        bExplode = true;
        bSpinning = false;
      }
    } // END BOMB
    
    else if (mode == SHIELD ) {
     
      // respond with a fun hello... don't effect game state
    
    }
    
    else if (mode == READY ) {
    
      // respond with a fun hello... don't effect game state
    
    }
  }


  if ( buttonDoubleClicked() ) {
    
    if ( mode == READY ) {
      // become the bomb
      mode = BOMB;
      
      // and start spinning
      resetSpin();
      bSpinning = true;
    }
  
    else if ( mode == BOMB ) {
      // start the spinning again
      resetSpin();
      bSpinning = true;
    }
    
    else if ( mode == SHIELD ) {
      // respond with a fun hello... don't effect game state
    }
  }

  if ( buttonLongPressed() ) {
    
    // reset
    resetToReady();
    
    // TODO: would be nice to hold a single one down to then reset all of the others...
    // this is less important than getting game play correct, and feels like the very last
    // thing to do
  }

  /*
     Game Logic
  */
  switch (mode) {

    case READY:
      // keep a look out for incoming signal saying we are a shield
      // if we get that message, change to shield mode
      FOREACH_FACE( f ) {
        if ( !isValueReceivedOnFaceExpired( f ) ) {
          byte neighbor = getLastValueReceivedOnFace( f );

          if (neighbor == BOMB) {
            mode = SHIELD;
          }
        }
      }
      break;

    case BOMB:
      // if we are spinning, spin the speed expected
      if ( bSpinning ) {
        if (!bombShowFaceTimer.isExpired()) {
          // hold on this face to show spark on this face
        }
        else {

          if ( bombTickTimer.isExpired() ) {
            bombTickTimer.set( getTickRate( bombClickCount ) );
            bombTickFace++;
            if ( bombTickFace >= FACE_COUNT ) {
              bombTickFace = 0;
            }
          }
        }
      }
      // if we've sparked, check to see if we've hit a sheild, or nothing
      // if nothing, then the spark becomes an explosion
      // if we hit a shield, the spark shows direction
      // if we have only one shield and we blow it away, we win
      break;

    case SHIELD:
      // if we see a spark lower our shield value by 1
      FOREACH_FACE( f ) {
        if ( !isValueReceivedOnFaceExpired( f ) ) {

          byte neighbor = getLastValueReceivedOnFace( f );
          bool didNeighborJustChange = didValueOnFaceChange( f );

          if ( neighbor == SPARK && didNeighborJustChange ) {

            // if our shield value is 0, become an explosion
            if ( shieldHealth == SHIELD_MIN_HEALTH ) {
              // explode this shield (maybe rainbow fun here)
              
            } else {
              shieldHealth--;
            }
          }
        }
      }
      // if we see an explosion, help animate that explosion
      break;

    default: break;
  }

  /*
     Display
  */
  switch (mode) {

    case READY:
      // display readiness or waiting
      setColor(BLUE);
      break;

    case BOMB:
      // display countdown
      // or spinner
      if ( bSpinning ) {
        setColor(OFF);
        setFaceColor( bombTickFace, ORANGE );
      }
      else {
        if ( bExplode ) {
          setColor(RED);
          setFaceColor( bombTickFace, WHITE );
          // show white if safe
          // red if out....
        }
      }
      // or spark
      // or explosion
      break;

    case SHIELD:
      // display shield level
      if( shieldHealth == SHIELD_MIN_HEALTH ) {
        // show explosion
        setFaceColor( (millis() / 40) % 6, makeColorHSB( ( millis() / 5) % 255, 255, 255) ); // ROTATING RAINBOW
      }
      else {
        setColor( getShieldColor( shieldHealth ) );
      }
      // or helping show internal explosion
      break;

    default: break;
  }

  /*
     Communications (Sending)
  */
  switch (mode) {

    case READY:
      // broadcast ready state... nothing to do here
      setValueSentOnAllFaces( READY );
      break;

    case BOMB:
      // broadcast bomb (let shields know you are a bomb)
      setValueSentOnAllFaces( BOMB );

      // if sparked, send in a direction
      if ( bExplode ) {
        setValueSentOnFace( SPARK, bombTickFace );
      }
      // once spark received, return to bomb broadcast
      break;

    case SHIELD:
      setValueSentOnAllFaces( SHIELD );
      // confirm spark received
      // share shield strength to the bomb
      break;

    default: break;
  }
}

/*
   Get ready to spin again
*/
void resetSpin() {
  // reset the spin variables
  // set spin speed to slowest
  // start the spin countdown
  bSpinning = false;
  bombClickCount = 0;
  bExplode = false;
}

/*
   Reset all of our variables for a new game
*/
void resetToReady() {
  mode = READY;
  shieldHealth = SHIELD_MAX_HEALTH;
  bSpinning = false;
  bombTickFace = 0;
  bombClickCount = 0;
  bExplode = false;
}

/*
   Return a frequency at which the bomb should spin
   Fine tune this to get more an more exciting
   The optimal tuning does not seem to be linear
*/
byte getTickRate(byte clickCount) {

  byte tickRate = 200;

  switch (clickCount) {
    case 0:  tickRate = 150; break;
    case 1:  tickRate = 120; break;
    case 2:  tickRate = 100; break;
    case 3:  tickRate =  80; break;
    case 4:  tickRate =  60; break;
    case 5:  tickRate =  50; break;
    case 6:  tickRate =  40; break;
    case 7:  tickRate =  35; break;
    case 8:  tickRate =  30; break;
    case 9:  tickRate =  25; break;
    case 10: tickRate =  20; break;
  }

  return tickRate;
}

Color getShieldColor( byte health ) {
  
  Color shieldColor = OFF;  // default

  switch( health ) {
    case 0: shieldColor = WHITE; break;                         // WHITE - TODO: replace w/ EXPLOSION
    case 1: shieldColor = makeColorHSB(  0, 255, 255); break;   // RED
    case 2: shieldColor = makeColorHSB( 25, 255, 255); break;   // ORANGE
    case 3: shieldColor = makeColorHSB( 50, 255, 255); break;   // YELLOW
    case 4: shieldColor = makeColorHSB( 75, 255, 255); break;   // GREEN
  }
  
  return shieldColor;
}


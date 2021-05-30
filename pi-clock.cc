// -*- mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
//
// Modified from 
//   https://github.com/hzeller/rpi-rgb-led-matrix/blob/master/examples-api-use/clock.cc
//
// This code is public domain - the source file is GPL v2
//  Note also the led-matrix library this depends on is GPL v2

#include "led-matrix.h"
#include "graphics.h"

#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <bits/stdc++.h>

using namespace rgb_matrix;

// internal structure to make code easier below
struct TZData {
  char textBuffer[256];
  char tzString[64];
  char tzDisplay[12];
  struct timespec next_time;
  struct tm tm;
  Color tzColor;
} ; 

volatile bool interrupt_received = false;
static void InterruptHandler(int signo) {
  interrupt_received = true;
}

static int usage(const char *progname) {
  fprintf(stderr, "usage: %s [options]\n", progname);
  fprintf(stderr, "Reads text from stdin and displays it. "
          "Empty string: clear screen\n");
  fprintf(stderr, "Options:\n");
  rgb_matrix::PrintMatrixFlags(stderr);
  fprintf(stderr,
          "\t-f <font-file>    : Use given font.\n"
          "\t-b <brightness>   : Sets brightness percent. Default: 100.\n"
          "\t-x <x-origin>     : X-Origin of displaying text (Default: 0)\n"
          "\t-y <y-origin>     : Y-Origin of displaying text (Default: 0)\n"
          "\t-S <spacing>      : Spacing pixels between letters (Default: 0)\n"
          );

  return 1;
}

static bool FullSaturation(const Color &c) {
    return (c.r == 0 || c.r == 255)
        && (c.g == 0 || c.g == 255)
        && (c.b == 0 || c.b == 255);
}

//
// Sorting routine - assumes 6 timezones
//  Probably should make this more generic but ...
//
TZData *tzPosition(int position, TZData *first, TZData *second, TZData *third,
                   TZData *fourth, TZData *fifth, TZData *sixth) {
  int n = 6 ;
  int inputArray[] = {first->tm.tm_hour,
                      second->tm.tm_hour,
                      third->tm.tm_hour,
                      fourth->tm.tm_hour,
                      fifth->tm.tm_hour,
                      sixth->tm.tm_hour };
  std::sort(inputArray, inputArray + n);

  //  looking for inputArray[position] to match - need to reverse it
  if (inputArray[5 - position] == first->tm.tm_hour) return first;
  if (inputArray[5 - position] == second->tm.tm_hour) return second;
  if (inputArray[5 - position] == third->tm.tm_hour) return third;
  if (inputArray[5 - position] == fourth->tm.tm_hour) return fourth;
  if (inputArray[5 - position] == fifth->tm.tm_hour) return fifth;

  // by default if get here it's the 6th position
  return sixth;
}

//
// Set color based on hour
//
Color tzColorSet(int hour, Color overnight, Color day, Color evening) {
  Color retVal = overnight;
  if (hour > 9) retVal = day;
  if (hour > 18) retVal = evening;
  return retVal;
}

//
// MAIN
//
int main(int argc, char *argv[]) {
  RGBMatrix::Options matrix_options;
  rgb_matrix::RuntimeOptions runtime_opt;
  if (!rgb_matrix::ParseOptionsFromFlags(&argc, &argv,
                                         &matrix_options, &runtime_opt)) {
    return usage(argv[0]);
  }

  // time format - default to H:M:S
  //   Probably should make this an argument...
  const char *tzFmtStr = "%H:%M:%S";

  // Create a number of base colors to utilize
  Color colorRed(255, 0, 0);
  Color colorGreen(0, 255, 0);
  Color colorBlue(0, 0, 255);
  Color colorWhite(255, 255, 255);
  Color colorRedGreen(255, 255, 0);
  Color colorGreenBlue(0, 255, 255);

  // background color will be black 
  Color bg_color(0, 0, 0);

  const char *bdf_font_file = NULL;
  int x_orig = 0;
  int y_orig = 0;
  int brightness = 100;
  int letter_spacing = 0;

  int opt;
  while ((opt = getopt(argc, argv, "x:y:f:b:S:")) != -1) {
    switch (opt) {
    case 'b': brightness = atoi(optarg); break;
    case 'x': x_orig = atoi(optarg); break;
    case 'y': y_orig = atoi(optarg); break;
    case 'f': bdf_font_file = strdup(optarg); break;
    case 'S': letter_spacing = atoi(optarg); break;
    default:
      return usage(argv[0]);
    }
  }

  if (bdf_font_file == NULL) {
    fprintf(stderr, "Need to specify BDF font-file with -f\n");
    return usage(argv[0]);
  }

  /*
   * Load font. This needs to be a filename with a bdf bitmap font.
   */
  rgb_matrix::Font font;
  if (!font.LoadFont(bdf_font_file)) {
    fprintf(stderr, "Couldn't load font '%s'\n", bdf_font_file);
    return 1;
  }
  if (brightness < 1 || brightness > 100) {
    fprintf(stderr, "Brightness is outside usable range.\n");
    return 1;
  }

  RGBMatrix *matrix = rgb_matrix::CreateMatrixFromOptions(matrix_options,
                                                          runtime_opt);
  if (matrix == NULL)
    return 1;

  matrix->SetBrightness(brightness);

  const bool all_extreme_colors = (brightness == 100)
      && FullSaturation(colorRed)
    && FullSaturation(bg_color);
  if (all_extreme_colors)
      matrix->SetPWMBits(1);

  int x = x_orig;
  int y = y_orig;

  FrameCanvas *offscreen = matrix->CreateFrameCanvas();

  //
  // What time zones are we using?
  //   These should probably be read in via config but ...
  // 
  TZData tz0;
  strcpy (tz0.tzString, "Europe/Paris");
  strcpy (tz0.tzDisplay, "CET");
  tz0.next_time.tv_sec = time(NULL);
  tz0.next_time.tv_nsec = 0;
  
  TZData tz1;
  strcpy (tz1.tzString, "Europe/London");
  strcpy (tz1.tzDisplay, "LHR");
  tz1.next_time.tv_sec = time(NULL);
  tz1.next_time.tv_nsec = 0;
  
  TZData tz2;
  strcpy (tz2.tzString, "America/New_York");
  strcpy (tz2.tzDisplay,"NYC");
  tz2.next_time.tv_sec = time(NULL);
  tz2.next_time.tv_nsec = 0;
    
  TZData tz3;
  strcpy (tz3.tzString, "America/Denver");
  strcpy (tz3.tzDisplay,"DEN");
  tz3.next_time.tv_sec = time(NULL);
  tz3.next_time.tv_nsec = 0;

  TZData tz4;
  strcpy (tz4.tzString, "America/Los_Angeles");
  strcpy (tz4.tzDisplay, "SEA");
  tz4.next_time.tv_sec = time(NULL);
  tz4.next_time.tv_nsec = 0;
  
  TZData tz5;
  strcpy (tz5.tzString, "Asia/Kolkata");
  strcpy (tz5.tzDisplay, "BLR");
  tz5.next_time.tv_sec = time(NULL);
  tz5.next_time.tv_nsec = 0;

  signal(SIGTERM, InterruptHandler);
  signal(SIGINT, InterruptHandler);

  while (!interrupt_received) {

      offscreen->Fill(bg_color.r, bg_color.g, bg_color.b);
      offscreen->Clear();

      // ============= Get TZ info for all 6 timezones ======================
      // ==  Set color as well based on hour (but you could hard code too)
      setenv("TZ", tz0.tzString, 1); tzset();
      localtime_r(&tz0.next_time.tv_sec, &tz0.tm);
      strftime(tz0.textBuffer, sizeof(tz0.textBuffer), tzFmtStr, &tz0.tm);
      tz0.tzColor = tzColorSet(tz0.tm.tm_hour, colorBlue, colorWhite, colorRed);
      
      setenv("TZ", tz1.tzString, 1); tzset();
      localtime_r(&tz1.next_time.tv_sec, &tz1.tm);
      strftime(tz1.textBuffer, sizeof(tz1.textBuffer), tzFmtStr, &tz1.tm);
      tz1.tzColor = tzColorSet(tz1.tm.tm_hour, colorBlue, colorWhite, colorRed);
      
      setenv("TZ", tz2.tzString, 1); tzset();
      localtime_r(&tz2.next_time.tv_sec, &tz2.tm);
      strftime(tz2.textBuffer, sizeof(tz2.textBuffer), tzFmtStr, &tz2.tm);
      tz2.tzColor = tzColorSet(tz2.tm.tm_hour, colorBlue, colorWhite, colorRed);
      
      setenv("TZ", tz3.tzString, 1); tzset();
      localtime_r(&tz3.next_time.tv_sec, &tz3.tm);
      strftime(tz3.textBuffer, sizeof(tz3.textBuffer), tzFmtStr, &tz3.tm);
      tz3.tzColor = tzColorSet(tz3.tm.tm_hour, colorBlue, colorWhite, colorRed);
      
      setenv("TZ", tz4.tzString, 1); tzset();
      localtime_r(&tz4.next_time.tv_sec, &tz4.tm);
      strftime(tz4.textBuffer, sizeof(tz4.textBuffer), tzFmtStr, &tz4.tm);
      tz4.tzColor = tzColorSet(tz4.tm.tm_hour, colorBlue, colorWhite, colorRed);
      
      setenv("TZ", tz5.tzString, 1); tzset();
      localtime_r(&tz5.next_time.tv_sec, &tz5.tm);
      strftime(tz5.textBuffer, sizeof(tz5.textBuffer), tzFmtStr, &tz5.tm);
      tz5.tzColor = tzColorSet(tz5.tm.tm_hour, colorBlue, colorWhite, colorRed);

      // ============= Now Sort how we display ======================
      // == ordering is odd b/c of the single display channel so we have
      // == need to interlace the output to be 0,2,4 and then 1,3,5
      TZData *tzD0 = tzPosition(0, &tz0, &tz1, &tz2, &tz3, &tz4, &tz5);
      TZData *tzD2 = tzPosition(1, &tz0, &tz1, &tz2, &tz3, &tz4, &tz5);
      TZData *tzD4 = tzPosition(2, &tz0, &tz1, &tz2, &tz3, &tz4, &tz5);
      TZData *tzD1 = tzPosition(3, &tz0, &tz1, &tz2, &tz3, &tz4, &tz5);
      TZData *tzD3 = tzPosition(4, &tz0, &tz1, &tz2, &tz3, &tz4, &tz5);
      TZData *tzD5 = tzPosition(5, &tz0, &tz1, &tz2, &tz3, &tz4, &tz5);

      // set our base starting point
      x = x_orig;
      y = y_orig;

      // ============= First Row ======================
      // == keeping in mind row is spread across 2 panels
      // == if you change font/sizing you will need to play with spacing
      x = x_orig;
      rgb_matrix::DrawText(offscreen, font, x, y + font.baseline(), tzD0->tzColor, NULL, tzD0->tzDisplay, letter_spacing);
      x += font.CharacterWidth(100) * strlen(tzD0->tzDisplay) + 2;
      rgb_matrix::DrawText(offscreen, font, x, y + font.baseline(), tzD0->tzColor, NULL, tzD0->textBuffer, letter_spacing);
      x += font.CharacterWidth(100) * strlen(tzD0->textBuffer) - 4;

      rgb_matrix::DrawText(offscreen, font, x, y + font.baseline(), tzD1->tzColor, NULL, tzD1->tzDisplay, letter_spacing);
      x += font.CharacterWidth(100) * strlen(tzD1->tzDisplay) + 2;
      rgb_matrix::DrawText(offscreen, font, x, y + font.baseline(), tzD1->tzColor, NULL, tzD1->textBuffer, letter_spacing);
      y += font.height();

      // ============= Second Row ======================
      x = x_orig;
      rgb_matrix::DrawText(offscreen, font, x, y + font.baseline(), tzD2->tzColor, NULL, tzD2->tzDisplay, letter_spacing);
      x += font.CharacterWidth(100) * strlen(tzD2->tzDisplay) + 2;
      rgb_matrix::DrawText(offscreen, font, x, y + font.baseline(), tzD2->tzColor, NULL, tzD2->textBuffer, letter_spacing);
      x += font.CharacterWidth(100) * strlen(tzD2->textBuffer) - 4;

      rgb_matrix::DrawText(offscreen, font, x, y + font.baseline(), tzD3->tzColor, NULL, tzD3->tzDisplay, letter_spacing);
      x += font.CharacterWidth(100) * strlen(tzD3->tzDisplay) + 2;
      rgb_matrix::DrawText(offscreen, font, x, y + font.baseline(), tzD3->tzColor, NULL, tzD3->textBuffer, letter_spacing);
      y += font.height();

      // ============= Third Row ======================
      x = x_orig;
      rgb_matrix::DrawText(offscreen, font, x, y + font.baseline(), tzD4->tzColor, NULL, tzD4->tzDisplay, letter_spacing);
      x += font.CharacterWidth(100) * strlen(tzD4->tzDisplay) + 2;
      rgb_matrix::DrawText(offscreen, font, x, y + font.baseline(), tzD4->tzColor, NULL, tzD4->textBuffer, letter_spacing);
      x += font.CharacterWidth(100) * strlen(tzD4->textBuffer) - 4;

      rgb_matrix::DrawText(offscreen, font, x, y + font.baseline(), tzD5->tzColor, NULL, tzD5->tzDisplay, letter_spacing);
      x += font.CharacterWidth(100) * strlen(tzD5->tzDisplay) + 2;
      rgb_matrix::DrawText(offscreen, font, x, y + font.baseline(), tzD5->tzColor, NULL, tzD5->textBuffer, letter_spacing);
      y += font.height();

      // Wait until we're ready to show it.
      clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &tz0.next_time, NULL);

      // Atomic swap with double buffer
      offscreen = matrix->SwapOnVSync(offscreen);

      // adjust second clock for all timezones and repeat
      tz0.next_time.tv_sec += 1;
      tz1.next_time.tv_sec += 1;
      tz2.next_time.tv_sec += 1;
      tz3.next_time.tv_sec += 1;
      tz4.next_time.tv_sec += 1;
      tz5.next_time.tv_sec += 1;
  }

  // Finished. Shut down the RGB matrix.
  matrix->Clear();
  delete matrix;

  write(STDOUT_FILENO, "\n", 1);  // Create a fresh new line after ^C on screen
  return 0;
}

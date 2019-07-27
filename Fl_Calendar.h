/*
  The simple Calendar widget for FLTK.
  Created by Nikita Egorov
*/
#ifndef __FL_CALENDAR_H__
#define __FL_CALENDAR_H__

#include <time.h>
#include "FL/Fl_Group.H"

class Fl_Double_Window;
class Fl_Input;
class Fl_Button;
class Fl_Calendar_Box;

struct calendar_widgets;

class Fl_Calendar : public Fl_Group {
public:
    Fl_Calendar(int X, int Y, int W, int H, const char* l = 0);
    void value(time_t date);
    time_t value() const;

protected:
    int handle(int event);       
    
    void on_button(Fl_Widget* b);
    void show_calendar_wnd(time_t date);

    static void on_button(Fl_Widget* b, void* d);
    static Fl_Calendar_Box* popup_wnd_;
    static calendar_widgets* widgets_;

    Fl_Input* input_;
    Fl_Button* button_;
    time_t value_;
};

#endif // __FL_CALENDAR_H__

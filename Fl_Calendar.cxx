/*
  The simple Calendar widget for FLTK.
  Created by Nikita Egorov
*/
#include "Fl_Calendar.h"

#include "FL/Fl.H"
#include "FL/fl_draw.H"
#include "FL/Fl_Button.H"
#include "FL/Fl_Table.H"
#include "FL/Fl_Input.H"
#include "FL/Fl_Box.H"
#include "FL/Fl_Spinner.H"
#include "FL/Fl_Double_Window.H"
#include "FL/x.H"
#include <locale.h>

#define SECSPERDAY (60 * 60 * 24)

class Fl_Days_Table : public Fl_Table {
public:
    Fl_Days_Table(int X, int Y, int W, int H, const char* l = 0);

    void draw_cell(TableContext context, int R, int C, int X, int Y, int W, int H);

    int handle(int event);

    void value(const time_t* t) {
        day_ = *t;
    }

    const time_t* value() const {
        return &day_;
    }

    void update_table() {
        tm d = *localtime(&day_);

        d.tm_mday = 1;
        auto t1 = mktime(&d); // first day of month
        d = *localtime(&t1);

        auto wday = (d.tm_wday + 6 - firstdayofweek_) % 7; // distance from first day of this week
        b_mon_ = t1 - SECSPERDAY * wday; // begin of week
                
        auto e_mon = b_mon_;
        int i = 0;
        for (; i < 6; i++) {
            auto m = e_mon + SECSPERDAY * 7;
            tm dd = *localtime(&m);
            if (dd.tm_mon != d.tm_mon) {
                break;
            }
            e_mon = m;
        }
        rows(i + 1);
        row_height_all((h() - col_header_height() - 5) / 6);
        auto days = (day_ - b_mon_) / SECSPERDAY;
        int R = int(days / 7);
        int C = days % 7;
        this->set_selection(R, C, R, C);
        redraw();
    }
    void event_callback2();
    // Actual static callback
    static void event_callback(Fl_Widget*, void* data);
protected:
    time_t  day_;
    time_t  b_mon_;
    int     firstdayofweek_;
};

typedef struct calendar_widgets {
    Fl_Calendar*     main;
    Fl_Button*       month_dec, *month_inc, *close;
    Fl_Spinner*      year;
    Fl_Box*          month;
    Fl_Days_Table*   table;
    tm               date_;
} calendar_widgets;

int Fl_Days_Table::handle(int event) {
    return Fl_Table::handle(event);
}

Fl_Days_Table::Fl_Days_Table(int X, int Y, int W, int H, const char* l)
    : Fl_Table(X, Y, W, H, l)
{   
    cols(7);
    col_width_all(w() / cols());
    day_ = time(NULL);
    col_header(1);
    col_header_height(20);
    update_table();
    callback(&event_callback, this);
    when(FL_WHEN_CHANGED | FL_WHEN_RELEASE);

    firstdayofweek_ = 0;
#ifdef _WIN32
    int err = ::GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_IFIRSTDAYOFWEEK | LOCALE_RETURN_NUMBER, (LPTSTR)&firstdayofweek_, sizeof(firstdayofweek_) / sizeof(TCHAR));
#endif
}

void Fl_Days_Table::event_callback2() {
    int R = callback_row(),                             // row where event occurred
        C = callback_col();                             // column where event occurred
    TableContext context = callback_context();          // which part of table

    if (context == CONTEXT_CELL) {
        set_selection(R, C, R, C);
        calendar_widgets* widgets = (calendar_widgets*)window()->user_data();
        if (widgets) {
            int ofs = R * 7 + C;
            auto tday = b_mon_ + SECSPERDAY * ofs;
            widgets->main->value(tday);
            window()->hide();
        }
    }
}

void Fl_Days_Table::event_callback(Fl_Widget*, void* data) {
    Fl_Days_Table *o = (Fl_Days_Table*)data;
    o->event_callback2();
}

void Fl_Days_Table::draw_cell(TableContext context, int R, int C, int X, int Y, int W, int H) {
    static char buf[40];

    switch (context) {
    case CONTEXT_STARTPAGE:             // Fl_Table telling us it's starting to draw page
        fl_font(FL_HELVETICA, 12);
        return;

    case CONTEXT_ROW_HEADER:            // Fl_Table telling us to draw row/col headers
        break;

    case CONTEXT_COL_HEADER:
        fl_push_clip(X, Y, W, H);
        {
            fl_draw_box(FL_THIN_UP_BOX, X, Y, W, H, FL_BACKGROUND_COLOR);
            fl_color(FL_FOREGROUND_COLOR);
            tm t = { 0 };
            t.tm_wday = (C + 1 + firstdayofweek_) % 7;
            strftime(buf, sizeof(buf), "%a", &t);
            fl_utf8from_mb(buf, sizeof(buf), buf, sizeof(buf));
            fl_draw(buf, X, Y, W, H, FL_ALIGN_CENTER);
        }
        fl_pop_clip();
        return;

    case CONTEXT_CELL:                  // Fl_Table telling us to draw cells
    {
        fl_push_clip(X, Y, W, H);

        int ofs = R * 7 + C;
        auto tday = b_mon_ + SECSPERDAY * ofs;
        tm day = *localtime(&tday);

        strftime(buf, 40, "%d", &day);
        tm today = *localtime(&day_);

        Fl_Color bg = is_selected(R, C) ? selection_color() : color();
        fl_rectf(X, Y, W, H, bg);

        if (day.tm_mon == today.tm_mon) {
            if (day.tm_mday == today.tm_mday)
                fl_color(FL_WHITE);
            else
                fl_color(FL_FOREGROUND_COLOR);
        }
        else
            fl_color(FL_DARK3);

        fl_draw(buf, X, Y, W, H, FL_ALIGN_CENTER);

        // BORDER
        fl_color(FL_BACKGROUND2_COLOR);
        if (C != cols() - 1) W++;
        if (R != rows() - 1) H++;
        fl_rect(X, Y, W, H);

        fl_pop_clip();
        return;
    }
    default:
        return;
    }
    //NOTREACHED
}

class Fl_Calendar_Box : public Fl_Double_Window
{
public:
   Fl_Calendar_Box(calendar_widgets* e)
      :Fl_Double_Window(250, 180)
   {
      widgets_ = e;
      box(FL_BORDER_BOX);
      { e->month_dec = new Fl_Button(5, 3, 20, 25, "@<");
      e->month_dec->labelcolor(FL_INACTIVE_COLOR);
      } // Fl_Button* e->month_dec
      { e->month = new Fl_Box(25, 3, 80, 25);
      e->month->box(FL_DOWN_BOX);
      e->month->color(FL_BACKGROUND2_COLOR);
      e->month->labelsize(12);
      e->month->align(Fl_Align(FL_ALIGN_CENTER | FL_ALIGN_INSIDE));
      } // Fl_Box* e->month
      { e->month_inc = new Fl_Button(105, 3, 20, 25, "@>");
      e->month_inc->labelcolor(FL_INACTIVE_COLOR);
      } // Fl_Button* e->month_inc
      { e->year = new Fl_Spinner(140, 4, 60, 24);
      e->year->minimum(2000);
      e->year->maximum(3000);
      e->year->textsize(12);
      e->year->align(Fl_Align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE));
      e->year->when(FL_WHEN_CHANGED);
      } // Fl_Spinner* e->year
      { e->close = new Fl_Button(220, 3, 25, 25, "@3plus");
      e->close->labelsize(12);
      e->close->labelcolor(FL_INACTIVE_COLOR);
      } // Fl_Button* e->close
      { e->table = new Fl_Days_Table(5, 31, 240, 144);
      e->table->box(FL_THIN_DOWN_FRAME);
      e->table->color((Fl_Color)55);
      e->table->selection_color(FL_SELECTION_COLOR);
      e->table->labeltype(FL_NORMAL_LABEL);
      e->table->labelfont(0);
      e->table->labelsize(12);
      e->table->labelcolor(FL_FOREGROUND_COLOR);
      e->table->align(Fl_Align(FL_ALIGN_TOP));
      e->table->when(FL_WHEN_RELEASE);
      e->table->end();
      } // Fl_Days_Table* e->table
      clear_border();
      set_modal();
      set_override();
      end();
   }
   void run() {
      show();
      Fl::grab(*this);
      while (shown())
         Fl::wait();
      Fl::grab(0);
   }
   int handle(int event) {
      switch (event)
      {
      case FL_MOVE:
         // workaround of strange behaviour :
         // Fl_Spinner(Fl_Input?) doesn't restore cursor when mouse leaves the widget
         if (Fl::belowmouse() != widgets_->year)
            cursor(FL_CURSOR_DEFAULT);
         break;
      }
      return Fl_Double_Window::handle(event);
   }
   calendar_widgets* widgets_;
};

static void on_close_calendar(Fl_Widget* w, void* data) {
    auto wnd = (Fl_Double_Window*)data;
    wnd->hide();
}

static void set_monyear(calendar_widgets* ws, int mon, int year=-1) {
    if (mon >= 0)
        ws->date_.tm_mon = mon;
    if (year >= 0) {
        ws->date_.tm_year = year - 1900;
        ws->year->value(year);
    }    
    if (mon >= 0) {
        char buf[128];
        strftime(buf, sizeof(buf), "%B", &ws->date_);
        fl_utf8from_mb(buf, sizeof(buf), buf, sizeof(buf));
        ws->month->copy_label(buf);
    }
    tm tm_d = ws->date_;
    time_t d = mktime(&tm_d);
    // detecting of wrong day of month (e.g. 31 feb, in fact it will be 03 mar)
    if (tm_d.tm_mon != ws->date_.tm_mon) {
        ws->date_.tm_mday -= tm_d.tm_mday;
        d = mktime(&ws->date_);
    }
    ws->table->value(&d);
    ws->table->update_table();
}

static void on_month_inc(Fl_Widget* w, void* data) {
    auto ws = (calendar_widgets*)data;
    int year = (int)ws->year->value();
    int mon = ((int)ws->date_.tm_mon + 1) % 12;
    if (mon == 0)
        ++year;
    set_monyear(ws, mon, year);
}

static void on_month_dec(Fl_Widget* w, void* data) {
    auto ws = (calendar_widgets*)data;
    int year = (int)ws->year->value();
    int mon = ((int)ws->date_.tm_mon + 11) % 12;
    if (mon == 11)
        --year;
    set_monyear(ws, mon, year);
}

static void on_year_changed(Fl_Widget* w, void* data) {
    auto ws = (calendar_widgets*)data;
    set_monyear(ws, -1, (int)ws->year->value());
}

Fl_Calendar_Box*  Fl_Calendar::popup_wnd_ = NULL;
calendar_widgets* Fl_Calendar::widgets_ = NULL;

void Fl_Calendar::show_calendar_wnd(time_t date) {
    if (!widgets_ && !popup_wnd_) {
        widgets_ = (calendar_widgets*)calloc(sizeof(calendar_widgets), 1);
        popup_wnd_ = new Fl_Calendar_Box(widgets_);

        widgets_->close->callback(on_close_calendar, popup_wnd_);
        widgets_->month_inc->callback(on_month_inc, widgets_);
        widgets_->month_dec->callback(on_month_dec, widgets_);
        widgets_->year->callback(on_year_changed, widgets_);
    }
    widgets_->main = this;
    widgets_->table->value(&date);
    widgets_->table->update_table();
        
    widgets_->date_ = *localtime(&date);

    set_monyear(widgets_, widgets_->date_.tm_mon, widgets_->date_.tm_year + 1900);

    popup_wnd_->position(window()->x()+ x(), window()->y() + y() + h());
    popup_wnd_->user_data(widgets_);
    popup_wnd_->run();
}

void Fl_Calendar::on_button(Fl_Widget* b) {
    show_calendar_wnd(value_);
}

void Fl_Calendar::on_button(Fl_Widget* w, void* d) {
    auto b = reinterpret_cast<Fl_Calendar*>(d);
    b->on_button(w);
}

Fl_Calendar::Fl_Calendar(int X, int Y, int W, int H, const char* l)
    : Fl_Group(X, Y, W, H, l)
{
    begin();
    input_ = new Fl_Input(X, Y, W - H, H);
    button_ = new Fl_Button(X + W - H, Y + 1, H, H - 2, "@2>");
    button_->callback(on_button, this);
    button_->box(FL_UP_BOX);
    time_t now = time(NULL);
    tm tnow = *localtime(&now);
    // truncate to day
    tnow.tm_hour = tnow.tm_min = tnow.tm_sec = 0;
    now = mktime(&tnow);
    value(now);
    end();
}

time_t Fl_Calendar::value() const {
    return value_;
}

void Fl_Calendar::value(time_t date) {
    value_ = date;
    tm day = *localtime(&date);
    char buf[128];
    strftime(buf, sizeof(buf), "%x", &day); // fixme : may be %Ex
    input_->value(buf);
}

int Fl_Calendar::handle(int event) {
    return Fl_Group::handle(event);
}

#ifdef TEST

int main(int argc, char* argv[]) {
    setlocale(LC_ALL, "");
    Fl_Double_Window* wnd = new Fl_Double_Window(400, 150, "Calendar");
    Fl_Calendar* cal = new Fl_Calendar(50, 20, 150, 25, "Date");
    wnd->end();
    wnd->show(argc, argv);
    int ret = Fl::run();
    time_t t = cal->value();
    printf(ctime(&t));
}

#endif

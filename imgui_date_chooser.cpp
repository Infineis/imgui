//- Common Code For All Addons needed just to ease inclusion as separate files in user code ----------------------
#include <imgui/imgui.h>
#undef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui/imgui_internal.h>
//-----------------------------------------------------------------------------------------------------------------


#include "imgui_date_chooser.h"
#include <time.h>               // very simple and common plain C header file (it's NOT the c++ <sys/time.h>). If not available it's probably better to implement it yourself rather than modifying this file.
#include <ctype.h>  // toupper()

namespace ImGui {


//struct tm
//{
//  int tm_sec;			/* Seconds.	[0-60] (1 leap second) */
//  int tm_min;			/* Minutes.	[0-59] */
//  int tm_hour;			/* Hours.	[0-23] */
//  int tm_mday;			/* Day.		[1-31] */
//  int tm_mon;			/* Month.	[0-11] */
//  int tm_year;			/* Year	- 1900.  */
//  int tm_wday;			/* Day of week.	[0-6] */
//  int tm_yday;			/* Days in year.[0-365]	*/
//  int tm_isdst;			/* DST.		[-1/0/1]*/

//# ifdef	__USE_BSD
//  long int tm_gmtoff;		/* Seconds east of UTC.  */
//  const char *tm_zone;		/* Timezone abbreviation.  */
//# else
//  long int __tm_gmtoff;		/* Seconds east of UTC.  */
//  const char *__tm_zone;	/* Timezone abbreviation.  */
//# endif
//};

inline static tm GetCurrentDate() {
    time_t now;time(&now);
    return *localtime(&now);
}
// Tip: we modify tm (its fields can even be negative!) and then we call this method to retrieve a valid date
inline static void RecalculateDateDependentFields(tm& date)    {
    date.tm_isdst=-1;   // This tries to detect day time savings too
    time_t tmp = mktime(&date);
    date=*localtime(&tmp);
}
/*inline static size_t DateToString(tm& d,char* buf,size_t bufferSize)  {
    return strftime(buf,bufferSize,
                    //"%a %d %b %Y"
                    "%d/%m/%Y"
                    ,&d);
}*/


inline static tm GetDateZero() {
    tm date;
    memset(&date,0,sizeof(tm));     // Mandatory for emscripten. Please do not remove!
    date.tm_isdst=-1;
    date.tm_sec=0;		//	 Seconds.	[0-60] (1 leap second)
    date.tm_min=0;		//	 Minutes.	[0-59]
    date.tm_hour=0;		//	 Hours.	[0-23]
    date.tm_mday=0;		//	 Day.		[1-31]
    date.tm_mon=0;		//	 Month.	[0-11]
    date.tm_year=0;		//	 Year	- 1900.
    date.tm_wday=0;     //	 Day of week.	[0-6]
    date.tm_yday=0;		//	 Days in year.[0-365]
    return date;
}
void SetDateZero(tm* date) {
    if (!date) return;
    *date = GetDateZero();
    return;
}
void SetDateToday(tm* date)    {
    if (!date) return;
    *date = GetCurrentDate();
    return;
}

// Not as good as the original version...
bool DateChooser(const char* label, tm& dateOut,const char* dateFormat,bool closeWhenMouseLeavesIt,bool* pSetStartDateToDateOutThisFrame,const char* leftArrow,const char* rightArrow,const char* upArrowString,const char* downArrowString)    {
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;
    const ImGuiID id = window->GetID(label);


    static tm d=GetDateZero();
    static ImGuiID lastOpenComboID = 0;
    if (pSetStartDateToDateOutThisFrame && *pSetStartDateToDateOutThisFrame) {
        *pSetStartDateToDateOutThisFrame=false;
        if (dateOut.tm_mday==0) dateOut = GetCurrentDate();
        d = dateOut;
        // move d at the first day of the month: mandatory fo the algo I'll use below
        d.tm_mday = 1;
        RecalculateDateDependentFields(d);  // now d.tm_wday is correct
    }
    else if (dateOut.tm_mday==0) {
        dateOut = GetCurrentDate();
        d = dateOut;
        // move d at the first day of the month: mandatory fo the algo I'll use below
        d.tm_mday = 1;
        RecalculateDateDependentFields(d);  // now d.tm_wday is correct
    }
    else if (d.tm_mday==0) {
        d = GetCurrentDate();
        // move d at the first day of the month: mandatory fo the algo I'll use below
        d.tm_mday = 1;
        RecalculateDateDependentFields(d);  // now d.tm_wday is correct
    }
    else if (d.tm_mday!=1) {
        // move d at the first day of the month: mandatory fo the algo I'll use below
        d.tm_mday = 1;
        RecalculateDateDependentFields(d);  // now d.tm_wday is correct
    }

    static const int nameBufferSize = 64;
    static char dayNames[7][nameBufferSize]={"","","","","","",""};
    static char monthNames[12][nameBufferSize]={"","","","","","",""};
    static char* pMonthNames[12]={&monthNames[0][0],&monthNames[1][0],&monthNames[2][0],&monthNames[3][0],&monthNames[4][0],&monthNames[5][0],&monthNames[6][0],&monthNames[7][0],&monthNames[8][0],&monthNames[9][0],&monthNames[10][0],&monthNames[11][0]};
    static int maxMonthWidthIndex = -1;
    static float maxMonthWidth = 100.0f;
    if (strlen(dayNames[0])==0) {
        tm tmp = GetDateZero();
        for (int i=0;i<7;i++)   {
            tmp.tm_wday=i;
            char* dayName = &dayNames[i][0];
            strftime(dayName,nameBufferSize,"%A",&tmp);
            //fprintf(stderr,"%s\n",dayNames[i]);
            // Warning: This part won't work for multibyte UTF-8 chars:
            // We want to keep the first letter only here, and we want it to be uppercase (because some locales provide it lowercase)
            if (strlen(dayName)==0) {
                static const char fallbacks[7]={'S','M','T','W','T','F','S'};
                dayName[0] = fallbacks[i];dayName[1] = '\0';
            }
            else {
                dayName[0]=toupper(dayName[0]);
                dayName[1]='\0';
            }
            //fprintf(stderr,"%s\n",dayNames[i]);

        }
        maxMonthWidth = 0;
        for (int i=0;i<12;i++)   {
            tmp.tm_mon=i;
            char* monthName = &monthNames[i][0];
            strftime(monthName,nameBufferSize,"%B",&tmp);
            //fprintf(stderr,"%s\n",monthNames[i]);
            // Warning: This part won't work for multibyte UTF-8 chars:
            // We want the first letter to be uppercase (because some locales provide it lowercase)
            if (strlen(monthName)==0) {
                static const char* fallbacks[12]={"January","February","March","April","May","June","July","August","September","October","November"," December"};
                strcpy(monthName,fallbacks[i]);
            }
            else monthName[0]=toupper(monthName[0]);
            //fprintf(stderr,"%s\n",monthNames[i]);
            float mw = ImGui::CalcTextSize(monthName).x;
            if (maxMonthWidth<mw) {
                maxMonthWidth = mw;
                maxMonthWidthIndex = i;
            }
        }
    }

    ImGuiStyle& style = g.Style;

    const char* arrowLeft= leftArrow ? leftArrow : ICON_MD_NAVIGATE_BEFORE;
    const char* arrowRight= rightArrow ? rightArrow : ICON_MD_NAVIGATE_NEXT;
    const float arrowLeftWidth  = ImGui::CalcTextSize(arrowLeft).x;
    const float arrowRightWidth = ImGui::CalcTextSize(arrowRight).x;

    static char currentText[128];strftime(currentText,128,dateFormat,&dateOut);
    const ImVec2 label_size = CalcTextSize(label, NULL, true);
    float widthAdder = 0;
    const float widthRequiredByCalendar = 2.f*arrowLeftWidth + 2.f*arrowRightWidth + ImGui::CalcTextSize(monthNames[maxMonthWidthIndex]).x + ImGui::CalcTextSize("9999").x + 180.f + widthAdder;
    const float widthRequiredByTextBox = label_size.x + ImGui::CalcTextSize(currentText).x;// + style.FramePadding.x*2.0f;
    const float w = widthRequiredByCalendar > widthRequiredByTextBox ? widthRequiredByCalendar : widthRequiredByTextBox;//ImGui::CalcItemWidth();

    const int height_in_items = 1 + 1 + 1 + 4;
    const int items_count = height_in_items;

    const float popup_height = (label_size.y + 2*style.ItemSpacing.y) * ImMin(items_count, height_in_items) + (style.FramePadding.y * 3);
    bool value_changed = false;

    ImGui::SetNextWindowSize(ImVec2(w,w));
    ImGui::SetNextWindowSizeConstraints(ImVec2(w,popup_height+30),ImVec2(w,popup_height+50));

    if (!ImGui::BeginCombo(label,currentText, ImGuiComboFlags_None))
        return value_changed;

    if (g.IO.MouseClicked[1])   {
        // reset date when user right clicks the date chooser header whe the dialog is open
        d = GetCurrentDate();
        // move d at the first day of the month: mandatory fo the algo I'll use below
        d.tm_mday = 1;
        RecalculateDateDependentFields(d);  // now d.tm_wday is correct
    }
    else if (lastOpenComboID!=id) {
        lastOpenComboID=id;
        if (dateOut.tm_mday==0) dateOut = GetCurrentDate();
        d = dateOut;
        // move d at the first day of the month: mandatory fo the algo I'll use below
        d.tm_mday = 1;
        RecalculateDateDependentFields(d);  // now d.tm_wday is correct
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, style.FramePadding);
    //ImGui::PushStyleColor(ImGuiCol_ChildBg, style.Colors[ImGuiCol_PopupBg]);
    ImGui::Spacing();

    static const ImVec4 transparent(1,1,1,0);
    ImGui::PushStyleColor(ImGuiCol_Button,transparent);

    static char yearString[12] = "";
    snprintf(yearString, sizeof(yearString), "%d", 1900+d.tm_year);
    
    //const float monthPartWidth = arrowLeftWidth + arrowRightWidth + ImGui::CalcTextSize(monthNames[d.tm_mon]).x;
    const float yearPartWidth = arrowLeftWidth + arrowRightWidth + ImGui::CalcTextSize(yearString).x + widthAdder*0.5f;

    const float oldWindowRounding = style.WindowRounding;
    style.WindowRounding = 0;

    ImGui::PushID(1234);
    if (ImGui::SmallButton(arrowLeft)) {d.tm_mon-=1;RecalculateDateDependentFields(d);}
    ImGui::SameLine();

    const char* month_label = monthNames[d.tm_mon];
    auto text_move_x = (maxMonthWidth - ImGui::CalcTextSize(month_label).x) / 2.0f;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + text_move_x);
    ImGui::TextUnformatted(monthNames[d.tm_mon]);    
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + text_move_x);
    if (ImGui::SmallButton(arrowRight)) {d.tm_mon+=1;RecalculateDateDependentFields(d);}
    ImGui::PopID();

    ImGui::SameLine(ImGui::GetWindowWidth()-yearPartWidth-ImGui::GetStyle().WindowPadding.x-ImGui::GetStyle().ItemSpacing.x*4.f);

    ImGui::PushID(1235);
    if (ImGui::SmallButton(arrowLeft)) {d.tm_year-=1;if(d.tm_year<0) d.tm_year=0;RecalculateDateDependentFields(d);}
    ImGui::SameLine();

    ImGui::Text("%d",1900+d.tm_year);
    ImGui::SameLine();
    if (ImGui::SmallButton(arrowRight)) {d.tm_year+=1;RecalculateDateDependentFields(d);}
    ImGui::PopID();

    ImGui::Spacing();

    const static int numDaysPerMonth[12]={31,28,31,30,31,30,31,31,30,31,30,31};
    int maxDayOfCurMonth = numDaysPerMonth[d.tm_mon];   // This could be calculated only when needed (but I guess it's fast in any case...)
    if (maxDayOfCurMonth==28)   {
        const int year = d.tm_year+1900;
        const bool bis = ((year%4)==0) && ((year%100)!=0 || (year%400)==0);
        if (bis) maxDayOfCurMonth=29;
    }
    static char curDayStr[4] = "";

    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,style.Colors[ImGuiCol_HeaderHovered]);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,style.Colors[ImGuiCol_HeaderActive]);

    ImGui::Separator();

    IM_ASSERT(d.tm_mday==1);    // Otherwise the algo does not work
    // Display items
    for (int dw=0;dw<7;dw++)    {
        ImGui::BeginGroup();
        ImGui::Text(" %s",dayNames[dw]);
        ImGui::Spacing();
        //-----------------------------------------------------------------------
        int curDay = dw-d.tm_wday;      // tm_wday is in [0,6]. For this to work here d must point to the first day of the month: i.e.: d.tm_mday = 1;
        for (int row=0;row<7;row++) {
            int cday=curDay+7*row;
            if (cday>=0 && cday<maxDayOfCurMonth)  {
                ImGui::PushID(row*10+dw);
                if (cday<9)
                    snprintf(curDayStr, sizeof(curDayStr), " %d", cday+1);
                else
                    snprintf(curDayStr, sizeof(curDayStr), "%d", cday + 1);

                const bool active_day = d.tm_year == dateOut.tm_year && d.tm_mon == dateOut.tm_mon && cday == dateOut.tm_mday - 1;
                if (active_day)
                {
                    const ImVec4& tc = style.Colors[ImGuiCol_Text];
                    const float l = (tc.x + tc.y + tc.z) * 0.33334f;
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(l * 2.f > 1 ? 1 : l * 2.f, l * .5f, l * .5f, tc.w));
                }
                if (ImGui::SmallButton(curDayStr)) {
                    //-------------------------
                    value_changed = true;
                    SetItemDefaultFocus();
                    dateOut = d;
                    dateOut.tm_mday = cday+1;
                    RecalculateDateDependentFields(dateOut);
                    d=dateOut;
                }
                if (active_day)
                    ImGui::PopStyleColor();
                ImGui::PopID();
            }
            else ImGui::TextUnformatted(" ");
        }
        ImGui::EndGroup();
        if (dw!=6)
            ImGui::SameLine(ImGui::GetWindowWidth()-(6-dw)*(ImGui::GetWindowWidth()/7.f));
    }

    style.WindowRounding = oldWindowRounding;
    ImGui::PopStyleColor(2);
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();

    bool mustCloseCombo = value_changed;
    if (closeWhenMouseLeavesIt && !mustCloseCombo) {
        const float distance = g.FontSize*1.75f;//1.3334f;//24;
        //fprintf(stderr,"%1.f",distance);
        ImVec2 pos = ImGui::GetWindowPos();pos.x-=distance;pos.y-=distance;
        ImVec2 size = ImGui::GetWindowSize();size.x+=2.f*distance;size.y+=2.f*distance;
        const ImVec2& mousePos = ImGui::GetIO().MousePos;
        if (mousePos.x<pos.x || mousePos.y<pos.y || mousePos.x>pos.x+size.x || mousePos.y>pos.y+size.y) {
            mustCloseCombo = true;
            //d = dateOut;
            //fprintf(stderr,"Leaving DateChooser\n");
        }
    }

    if (mustCloseCombo) ImGui::CloseCurrentPopup();
    ImGui::EndCombo();

    //if (value_changed) d.tm_mday=0;    //reset d
    return value_changed;

}

void TestDateChooser(const char* dateFormat,bool closeWhenMouseLeavesIt, const char* leftArrow, const char* rightArrow, const char* upArrowString, const char* downArrowString)  {
    static tm date=GetDateZero();
    if (DateChooser("DateChooser##id",date,dateFormat,closeWhenMouseLeavesIt,NULL,leftArrow,rightArrow,upArrowString,downArrowString)) {
        // Do something with date
    }
}

} // namespace ImGui

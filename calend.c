/*
	(С) Волков Максим 2019 ( Maxim.N.Volkov@ya.ru )
	
	Календарь v1.0  
	Приложение простого календаря.
	Алгоритм вычисления дня недели работает для любой даты григорианского календаря позднее 1583 года. 
	Григорианский календарь начал действовать в 1582 — после 4 октября сразу настало 15 октября. 
	
	Календарь от 1600 до 3000 года
	Функции перелистывания каленаря вверх-вниз - месяц, стрелками год
	При нажатии на название месяца устанавливается текущая дата
	
	
	v.1.1
	- исправлены переходы в при запуске из бстрого меню
	Lưu ý: không được để set_update_period(1, 0);
	Bản này không bị nhảy màn hình
	Có cái vẫn phải chờ 1 phút mới bấm tiếp số tiếp theo được
*/

#include "libbip.h"
#include "calend.h"
#define DEBUG_LOG

//	структура меню экрана календаря
struct regmenu_ menu_calend_screen = {
						55,
						1,
						0,
						dispatch_calend_screen,
						key_press_calend_screen, 
						calend_screen_job,
						0,
						show_calend_screen,
						0,
						0
					};

int main(int param0, char** argv){	//	переменная argv не определена
	show_calend_screen((void*) param0);
}

void show_calend_screen (void *param0){
struct calend_** 	calend_p = get_ptr_temp_buf_2(); 	//	указатель на указатель на данные экрана 
struct calend_ *	calend;								//	указатель на данные экрана
struct calend_opt_ 	calend_opt;							//tùy chọn lịch, cái này khác
//app_data_ thay bằng calend_

#ifdef DEBUG_LOG
log_printf(5, "[show_calend_screen] param0=%X; *temp_buf_2=%X; menu_overlay=%d", (int)param0, (int*)get_ptr_temp_buf_2(), get_var_menu_overlay());
log_printf(5, " #calend_p=%X; *calend_p=%X", (int)calend_p, (int)*calend_p);
#endif

if ( (param0 == *calend_p) && get_var_menu_overlay()){ // возврат из оверлейного экрана (входящий звонок, уведомление, будильник, цель и т.д.)

#ifdef DEBUG_LOG
	log_printf(5, "  #from overlay");
	log_printf(5, "\r\n");
#endif	

	calend = *calend_p;						//	указатель на данные необходимо сохранить для исключения 
											//	высвобождения памяти функцией reg_menu
	*calend_p = NULL;						//	обнуляем указатель для передачи в функцию reg_menu	

	// 	создаем новый экран, при этом указатели temp_buf_1 и temp_buf_2 были равны 0 и память не была высвобождена	
	reg_menu(&menu_calend_screen, 0); 		// 	menu_overlay=0
	
	*calend_p = calend;						//	восстанавливаем указатель на данные после функции reg_menu
	
	draw_month(0, calend->month, calend->year);
	
} else { 			// если запуск функции произошел из меню, 

#ifdef DEBUG_LOG
	log_printf(5, "  #from menu");
	log_printf(5, "\r\n");
#endif
	// создаем экран
	reg_menu(&menu_calend_screen, 0);

	// выделяем необходимую память и размещаем в ней данные
	*calend_p = (struct calend_ *)pvPortMalloc(sizeof(struct calend_));
	calend = *calend_p;		//	указатель на данные
	
	// очистим память под данные
	_memclr(calend, sizeof(struct calend_));
	
	calend->proc = param0;
	
	// запомним адрес указателя на функцию в которую необходимо вернуться после завершения данного экрана
	if ( param0 && calend->proc->elf_finish ) 			//	если указатель на возврат передан, то возвоащаемся на него
		calend->ret_f = calend->proc->elf_finish;
	else					//	если нет, то на циферблат
		calend->ret_f = show_watchface;
	
	struct datetime_ datetime;
	_memclr(&datetime, sizeof(struct datetime_));
	
		// получим текущую дату
	get_current_date_time(&datetime);
	
	calend->day 	= datetime.day;
	calend->month 	= datetime.month;
	calend->year 	= datetime.year;

	// считаем опции из flash памяти, если значение в флэш-памяти некорректное то берем первую схему
	// текущая цветовая схема хранится о смещению 0
	ElfReadSettings(calend->proc->index_listed, &calend_opt, OPT_OFFSET_CALEND_OPT, sizeof(struct calend_opt_));
	
	if (calend_opt.color_scheme < COLOR_SCHEME_COUNT) 
			calend->color_scheme = calend_opt.color_scheme;
	else 
			calend->color_scheme = 0;
	calend->screen = 1;
	day = calend->day;
	month = calend->month;
	year = calend->year;
	//draw_month(calend->day, calend->month, calend->year);
	draw_month();
	number = 0;
	//number_time = 0;
}

// при бездействии погасить подсветку и не выходить
set_display_state_value(8, 1);
set_display_state_value(2, 1);

// таймер на job на 20с где выход.
//set_update_period(1, INACTIVITY_PERIOD);

}

void draw_month(){
struct calend_** 	calend_p = get_ptr_temp_buf_2(); 		//	указатель на указатель на данные экрана 
struct calend_ *	calend = *calend_p;						//	указатель на данные экрана


/*
 0:	CALEND_COLOR_BG					фон календаря
 1:	CALEND_COLOR_MONTH				цвет названия текущего месяца
 2:	CALEND_COLOR_YEAR				цвет текущего года
 3:	CALEND_COLOR_WORK_NAME			цвет названий дней будни
 4: CALEND_COLOR_HOLY_NAME_BG		фон	 названий дней выходные
 5:	CALEND_COLOR_HOLY_NAME_FG		цвет названий дней выходные
 6:	CALEND_COLOR_SEPAR				цвет разделителей календаря
 7:	CALEND_COLOR_NOT_CUR_WORK		цвет чисел НЕ текущего месяца будни
 8:	CALEND_COLOR_NOT_CUR_HOLY_BG	фон  чисел НЕ текущего месяца выходные
 9:	CALEND_COLOR_NOT_CUR_HOLY_FG	цвет чисел НЕ текущего месяца выходные
10:	CALEND_COLOR_CUR_WORK			цвет чисел текущего месяца будни
11:	CALEND_COLOR_CUR_HOLY_BG		фон  чисел текущего месяца выходные
12:	CALEND_COLOR_CUR_HOLY_FG		цвет чисел текущего месяца выходные
13: CALEND_COLOR_TODAY_BG			фон  чисел текущего дня; 		bit 31 - заливка: =0 заливка цветом фона, =1 только рамка, фон как у числа не текущего месяца 
14: CALEND_COLOR_TODAY_FG			цвет чисел текущего дня
*/


static unsigned char short_color_scheme[COLOR_SCHEME_COUNT][15] = 	
/* черная тема без выделения выходных*/		{//		0				1			2			3			4			5			6
											 {COLOR_SH_BLACK, COLOR_SH_YELLOW, COLOR_SH_AQUA, COLOR_SH_WHITE, COLOR_SH_RED, COLOR_SH_WHITE, COLOR_SH_WHITE, 
											 COLOR_SH_GREEN, COLOR_SH_BLACK, COLOR_SH_AQUA, COLOR_SH_YELLOW, COLOR_SH_BLACK, COLOR_SH_WHITE, COLOR_SH_YELLOW, COLOR_SH_BLACK}, 
											 //		7				8			9			10			11			12			13			14 
											 //		0				1			2			3			4			5			6
/* белая тема без выделения выходных*/		{COLOR_SH_WHITE, COLOR_SH_BLACK, COLOR_SH_BLUE, COLOR_SH_BLACK, COLOR_SH_RED, COLOR_SH_WHITE, COLOR_SH_BLACK, 
											 COLOR_SH_BLUE, COLOR_SH_WHITE, COLOR_SH_AQUA, COLOR_SH_BLACK, COLOR_SH_WHITE, COLOR_SH_RED, COLOR_SH_BLUE, COLOR_SH_WHITE},
											 //		7				8			9			10			11			12			13			14 	 
											 //		0				1			2			3			4			5			6
/* черная тема с выделением выходных*/		{COLOR_SH_BLACK, COLOR_SH_YELLOW, COLOR_SH_AQUA, COLOR_SH_WHITE, COLOR_SH_RED, COLOR_SH_WHITE, COLOR_SH_WHITE, 
											 COLOR_SH_GREEN, COLOR_SH_RED, COLOR_SH_AQUA, COLOR_SH_YELLOW, COLOR_SH_RED, COLOR_SH_WHITE, COLOR_SH_AQUA, COLOR_SH_BLACK}, 
											 //		7				8			9			10			11			12 			13			14 
											 //		0				1			2			3			4			5			6
/* белая тема с выделением выходных*/		{COLOR_SH_WHITE, COLOR_SH_BLACK, COLOR_SH_BLUE, COLOR_SH_BLACK, COLOR_SH_RED, COLOR_SH_WHITE, COLOR_SH_BLACK, 
											 COLOR_SH_BLUE, COLOR_SH_RED, COLOR_SH_BLUE, COLOR_SH_BLACK, COLOR_SH_RED, COLOR_SH_BLACK, COLOR_SH_BLUE, COLOR_SH_WHITE},
											 //		7				8			9			10			11			12			13			14 	 
											//		0				1			2			3			4			5			6
/* черная тема без выделения выходных*/		{COLOR_SH_BLACK, COLOR_SH_YELLOW, COLOR_SH_AQUA, COLOR_SH_WHITE, COLOR_SH_RED, COLOR_SH_WHITE, COLOR_SH_WHITE, 
/*с рамкой выделения сегодняшнего дня*/	     COLOR_SH_GREEN, COLOR_SH_BLACK, COLOR_SH_AQUA, COLOR_SH_YELLOW, COLOR_SH_BLACK, COLOR_SH_WHITE, COLOR_SH_AQUA|(1<<7), COLOR_SH_BLACK},
											 //		7				8			9			10			11			12			13			14 	 
											
											};

int color_scheme[COLOR_SCHEME_COUNT][15];


for (unsigned char i=0;i<COLOR_SCHEME_COUNT;i++)
	for (unsigned char j=0;j<15;j++){
	color_scheme[i][j]  = (((unsigned int)short_color_scheme[i][j]&(unsigned char)COLOR_SH_MASK)&COLOR_SH_RED)  ?COLOR_RED   :0;	//	составляющая красного цвета
	color_scheme[i][j] |= (((unsigned int)short_color_scheme[i][j]&(unsigned char)COLOR_SH_MASK)&COLOR_SH_GREEN)?COLOR_GREEN :0;	//	составляющая зеленого цвета
	color_scheme[i][j] |= (((unsigned int)short_color_scheme[i][j]&(unsigned char)COLOR_SH_MASK)&COLOR_SH_BLUE) ?COLOR_BLUE  :0;	//	составляющая синего цвета
	color_scheme[i][j] |= (((unsigned int)short_color_scheme[i][j]&(unsigned char)(1<<7))) ?(1<<31) :0;				//	для рамки	
}

char text_buffer[24];
char *weekday_string_ru[] = {"??", "T2", "T3", "T4", "T5", "T6", "T7", "CN"};
char *weekday_string_en[] = {"??", "Mo", "Tu", "We", "Th", "Fr", "Sa", "Su"};
char *weekday_string_it[] = {"??", "Lu", "Ma", "Me", "Gi", "Ve", "Sa", "Do"};
char *weekday_string_fr[] = {"??", "Lu", "Ma", "Me", "Je", "Ve", "Sa", "Di"};
char *weekday_string_de[] = {"??", "Mo", "Di", "Mi", "Do", "Fr", "Sa", "So"};

char *weekday_string_short_ru[] = {"?", "2", "3", "4", "5", "6", "7", "8"};
char *weekday_string_short_en[] = {"?", "M", "T", "W", "T", "F", "S", "S"};
char *weekday_string_short_it[] = {"?", "L", "M", "M", "G", "V", "S", "D"};
char *weekday_string_short_fr[] = {"?", "M", "T", "W", "T", "F", "S", "S"};
char *weekday_string_short_de[] = {"?", "M", "D", "M", "D", "F", "S", "S"};

char *monthname_ru[] = {
	     "???",		
	     "Tháng 1", 		"Tháng 2", 	"Tháng 3", 	"Tháng 4",
		 "Tháng 5", 		"Tháng 6",		"Tháng 7", 	"Tháng 8", 
		 "Tháng 9", 	"Tháng 10", 	"Tháng 11",	"Tháng 12"};

char *monthname_en[] = {
	     "???",		
	     "January", 	"February", 	"March", 		"April", 	
		 "May", 		"June",			"July", 		"August",
		 "September", 	"October", 		"November",		"December"};

char *monthname_it[] = {
	     "???",		
	     "Gennaio", 	"Febbraio", "Marzo", 	"Aprile", 
		 "Maggio", 		"Giugno",   "Luglio", 	"Agosto", 
		 "Settembre", 	"Ottobre", 	"Novembre", "Dicembre"};
		 
char *monthname_fr[] = {
	     "???",		
	     "Janvier",		"Février",	"Mars",		"Avril", 
		 "Mai", 		"Juin", 	"Juillet",	"Août", 
		 "Septembre", 	"Octobre", 	"Novembre", "Décembre"};
char *monthname_de[] = {
	     "???",		
	     "Januar", 		"Februar", 	"Maerz", 		"April", 	
		 "Mai", 		"Juni", 	"Juli",		"August", 	
		 "September", 	"Oktober", 	"November", 	"Dezember"};


unsigned char day_month[] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

char**	weekday_string;
char**	weekday_string_short;
char**	monthname;

switch (get_selected_locale()){
		case locale_ru_RU:{
			weekday_string = weekday_string_ru;
			weekday_string_short = weekday_string_short_ru;
			monthname = monthname_ru;
			break;
		}
		case locale_it_IT:{
			weekday_string = weekday_string_it;
			weekday_string_short = weekday_string_short_it;
			monthname = monthname_it;
			break;
		}
		case locale_fr_FR:{
			weekday_string = weekday_string_fr;
			weekday_string_short = weekday_string_short_fr;
			monthname = monthname_fr;
			break;
		}
		case locale_de_DE:{
			weekday_string = weekday_string_de;
			weekday_string_short = weekday_string_short_de;
			monthname = monthname_de;
			break;
		}
		default:{
			weekday_string = weekday_string_en;
			weekday_string_short = weekday_string_short_en;
			monthname = monthname_en;
			break;
		}
	}

_memclr(&text_buffer, 24);

set_bg_color(color_scheme[calend->color_scheme][CALEND_COLOR_BG]);	//	фон календаря
fill_screen_bg();

set_graph_callback_to_ram_1();
load_font(); // подгружаем шрифты

_sprintf(&text_buffer[0], " %d", year);
int month_text_width = text_width(monthname[month]);
int year_text_width  = text_width(&text_buffer[0]);

set_fg_color(color_scheme[calend->color_scheme][CALEND_COLOR_MONTH]);		//	цвет месяца
text_out(monthname[month], (176-month_text_width-year_text_width)/2 ,5); 	// 	вывод названия месяца

set_fg_color(color_scheme[calend->color_scheme][CALEND_COLOR_YEAR]);		//	цвет года
text_out(&text_buffer[0],  (176+month_text_width-year_text_width)/2 ,5); 	// 	вывод года

text_out("←", 5		 ,5); 		// вывод стрелки влево
text_out("→", 176-5-text_width("→"),5); 		// вывод стрелки вправо

int calend_name_height = get_text_height();

set_fg_color(color_scheme[calend->color_scheme][CALEND_COLOR_SEPAR]);
draw_horizontal_line(CALEND_Y_BASE, H_MARGIN, 176-H_MARGIN);	// Верхний разделитель названий дней недели
draw_horizontal_line(CALEND_Y_BASE+1+V_MARGIN+calend_name_height+1+V_MARGIN, H_MARGIN, 176-H_MARGIN);	// Нижний  разделитель названий дней недели
//draw_horizontal_line(175, H_MARGIN, 176-H_MARGIN);	// Нижний  разделитель месяца
 
// Названия дней недели
for (unsigned i=1; (i<=7);i++){
	if ( i>5 ){		//	выходные
		set_bg_color(color_scheme[calend->color_scheme][CALEND_COLOR_HOLY_NAME_BG]);
		set_fg_color(color_scheme[calend->color_scheme][CALEND_COLOR_HOLY_NAME_FG]);
	} else {		//	рабочие
		set_bg_color(color_scheme[calend->color_scheme][CALEND_COLOR_BG]);	
		set_fg_color(color_scheme[calend->color_scheme][CALEND_COLOR_WORK_NAME]);
	}
	
	
	// отрисовка фона названий выходных    
	int pos_x1 = H_MARGIN +(i-1)*(WIDTH  + H_SPACE);
	int pos_y1 = CALEND_Y_BASE+V_MARGIN+1;
	int pos_x2 = pos_x1 + WIDTH;
	int pos_y2 = pos_y1 + calend_name_height;

	// фон для каждого названия дня недели
	draw_filled_rect_bg(pos_x1, pos_y1, pos_x2, pos_y2);
	
	// вывод названий дней недели. если ширина названия больше чем ширина поля, выводим короткие названия
	if (text_width(weekday_string[1]) <= (WIDTH - 2))
		text_out_center(weekday_string[i], pos_x1 + WIDTH/2, pos_y1 + (calend_name_height-get_text_height())/2 );	
	else 
		text_out_center(weekday_string_short[i], pos_x1 + WIDTH/2, pos_y1 + (calend_name_height-get_text_height())/2 );	
}


int calend_days_y_base = CALEND_Y_BASE+1+V_MARGIN+calend_name_height+V_MARGIN+1;

if (isLeapYear(year)>0) day_month[2]=29;

unsigned char d=wday(1,month, year);
unsigned char m=month;
if (d>1) {
     m=(month==1)?12:month-1;
     d=day_month[m]-d+2;
	}

// числа месяца
for (unsigned i=1; (i<=7*6);i++){
     
	 unsigned char row = (i-1)/7;
     unsigned char col = (i-1)%7+1;
         
    _sprintf (&text_buffer[0], "%2.0d", d);
	
	int bg_color = 0;
	int fg_color = 0;
	int frame_color = 0; 	// цветрамки
	int frame    = 0; 		// 1-рамка; 0 - заливка
	
	// если текущий день текущего месяца
	if ( (m==month)&&(d==day) ){
		
		if ( color_scheme[calend->color_scheme][CALEND_COLOR_TODAY_BG] & (1<<31) ) {// если заливка отключена  только рамка
			
			// цвет рамки устанавливаем CALEND_COLOR_TODAY_BG, фон внутри рамки и цвет текста такой же как был
			frame_color = (color_scheme[calend->color_scheme][CALEND_COLOR_TODAY_BG &COLOR_MASK]);
			// рисуем рамку
			frame = 1;
			
			if ( col > 5 ){ // если выходные 
				bg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_CUR_HOLY_BG]); 
				fg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_CUR_HOLY_FG]);
			} else {		//	если будни
				bg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_BG]); 
				fg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_CUR_WORK]);
			};
			
		} else { 						// если включена заливка	
			if ( col > 5 ){ // если выходные 
				bg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_TODAY_BG] & COLOR_MASK); 
				fg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_TODAY_FG]);
			} else {		//	если будни
				bg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_TODAY_BG] &COLOR_MASK); 
				fg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_TODAY_FG]);
			};
		};
		

	} else {
	if ( col > 5 ){  // если выходные 
		if (month == m){
			bg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_CUR_HOLY_BG]); 
			fg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_CUR_HOLY_FG]);
		} else {
			bg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_NOT_CUR_HOLY_BG]); 
			fg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_NOT_CUR_HOLY_FG]);
		}
	} else {		//	если будни
		if (month == m){
			bg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_BG]); 
			fg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_CUR_WORK]);
		} else {
			bg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_BG]); 
			fg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_NOT_CUR_WORK]);
		}
	}
	}
	
	
	
	//  строка: от 7 до 169 = 162рх в ширину 7 чисел по 24рх на число 7+(22+2)*6+22+3
	//  строки: от 57 до 174 = 117рх в высоту 6 строк по 19рх на строку 1+(17+2)*5+18
	
	// отрисовка фона числа
	int pos_x1 = H_MARGIN +(col-1)*(WIDTH  + H_SPACE);
	int pos_y1 = calend_days_y_base + V_MARGIN + row *(HEIGHT + V_SPACE)+1;
	int pos_x2 = pos_x1 + WIDTH;
	int pos_y2 = pos_y1 + HEIGHT;	
	
	if (frame){
		// выводим число
		set_bg_color(bg_color);
		set_fg_color(fg_color);
		text_out_center(&text_buffer[0], pos_x1+WIDTH/2, pos_y1+(HEIGHT-get_text_height())/2);
		
		//	рисуем рамку
		set_fg_color(frame_color);
		draw_rect(pos_x1, pos_y1, pos_x2-1, pos_y2-1);	
	} else {
		// рисуем заливку
		set_bg_color(bg_color);
		draw_filled_rect_bg(pos_x1, pos_y1, pos_x2, pos_y2);
		
		// выводим число
		set_fg_color(fg_color);
		text_out_center(&text_buffer[0], pos_x1+WIDTH/2, pos_y1+(HEIGHT-get_text_height())/2);
	};
	
	
	
		
	if ( d < day_month[m] ) {
		d++;
	} else {
		d=1; 
		m=(m==12)?1:(m+1);
	}
}



};

unsigned char wday(unsigned int day,unsigned int month,unsigned int year)
{
    signed int a = (14 - month) / 12;
    signed int y = year - a;
    signed int m = month + 12 * a - 2;
    return 1+(((day + y + y / 4 - y / 100 + y / 400 + (31 * m) / 12) % 7) +6)%7;
}

unsigned char isLeapYear(unsigned int year){
    unsigned char result = 0;
    if ( (year % 4)   == 0 ) result++;
    if ( (year % 100) == 0 ) result--;
    if ( (year % 400) == 0 ) result++;
return result;
}

void key_press_calend_screen(){
	struct calend_** 	calend_p = get_ptr_temp_buf_2(); 		//	указатель на указатель на данные экрана 
	struct calend_ *	calend = *calend_p;			//	указатель на данные экрана
	
	show_menu_animate(calend->ret_f, (unsigned int)show_calend_screen, ANIMATE_RIGHT);	
};

void numpad(){
	//struct app_data_** 	app_data_p = get_ptr_temp_buf_2(); 	//	указатель на указатель на данные экрана 
	//struct app_data_ *	app_data = *app_data_p;				//	указатель на данные экрана
struct calend_** 	calend_p = get_ptr_temp_buf_2(); 		//	указатель на указатель на данные экрана 
struct calend_ *	calend = *calend_p;						//	указатель на данные экрана

  //set_backlight_state(1);
	//set_bg_color(COLOR_BLACK);
	//fill_screen_bg();
  
  char show_number[10];
		_sprintf(show_number, "%d", number);
	/*if (number > 999999999) { 
		set_bg_color(COLOR_BLACK);
		set_fg_color(COLOR_RED);
		text_out("Nhập số <10 chữ số", 10 , 5);
		number = number / 100;
	} else {*/
		set_bg_color(COLOR_BLACK);
		set_fg_color(COLOR_AQUA);
		text_out(show_number, 10 , 5);
	

//set_update_period(1, 10);  
//repaint_screen_lines(0, 176);    
}

void warningpad(){
	//struct app_data_** 	app_data_p = get_ptr_temp_buf_2(); 	//	указатель на указатель на данные экрана 
	//struct app_data_ *	app_data = *app_data_p;				//	указатель на данные экрана
struct calend_** 	calend_p = get_ptr_temp_buf_2(); 		//	указатель на указатель на данные экрана 
struct calend_ *	calend = *calend_p;						//	указатель на данные экрана

  //set_backlight_state(1);
	set_bg_color(COLOR_BLACK);
  fill_screen_bg();
  
  //Hàng đầu tiên: 1, 2, 3, Xóa (y: 26 - 26+50)
 set_bg_color(COLOR_RED);
  set_fg_color(COLOR_BLACK);
  draw_filled_rect_bg(0, 26, 132, 176);
  set_bg_color(COLOR_RED);
  text_out("Vui lòng nhập \n nhỏ hơn \n2.100.000.000", 10, 30);
  
    set_bg_color(COLOR_GREEN);
  set_fg_color(COLOR_WHITE);
  draw_filled_rect_bg(132, 26, 176, 76);
   set_bg_color(COLOR_GREEN);
  text_out("<", 152, 30);
    //Hàng thứ hai: 4, 5, 6, 0 (y: 76 - 76+50)
   set_bg_color(COLOR_AQUA);
  set_fg_color(COLOR_BLACK);
  draw_filled_rect_bg(132, 76, 176, 126);
  set_bg_color(COLOR_AQUA);
  text_out("=((", 152, 80);
  //Hàng Thứ 3: 7, 8, 9, > (y: 76 - 76+50)
    set_bg_color(COLOR_GREEN);
  set_fg_color(COLOR_WHITE);
  draw_filled_rect_bg(132, 126, 176, 176);
   set_bg_color(COLOR_GREEN);
  text_out("=", 152, 130);

set_update_period(1, 60000);  
repaint_screen_lines(0, 176);    
}

/*Bình luận: Mặc dù Bin đã có đổi sang định dạng long rồi nhưng vì hạn chế của Phần cứng nên vẫn không hiển thị được
https://nguyenvanhieu.vn/kieu-du-lieu-trong-c/
int	2 or 4 bytes	-32,768 tới 32,767 hoặc -2,147,483,648 tới 2,147,483,647
long	8 bytes	-9223372036854775808 tới 9223372036854775807
*/

void keypad(){
	//struct app_data_** 	app_data_p = get_ptr_temp_buf_2(); 	//	указатель на указатель на данные экрана 
	//struct app_data_ *	app_data = *app_data_p;				//	указатель на данные экрана
struct calend_** 	calend_p = get_ptr_temp_buf_2(); 		//	указатель на указатель на данные экрана 
struct calend_ *	calend = *calend_p;						//	указатель на данные экрана

  //set_backlight_state(1);
	set_bg_color(COLOR_BLACK);
  fill_screen_bg();
  
  //Hàng đầu tiên: 1, 2, 3, Xóa (y: 26 - 26+50)
 set_bg_color(COLOR_AQUA);
  set_fg_color(COLOR_BLACK);
  draw_filled_rect_bg(0, 26, 44, 76);
  set_bg_color(COLOR_AQUA);
  text_out("1", 20, 30);
   set_bg_color(COLOR_GREEN);
  set_fg_color(COLOR_WHITE);
  draw_filled_rect_bg(44, 26, 88, 76);
  set_bg_color(COLOR_GREEN);
  text_out("2", 64, 30);
   set_bg_color(COLOR_AQUA);
  set_fg_color(COLOR_BLACK);
  draw_filled_rect_bg(88, 26, 132, 76);
  set_bg_color(COLOR_AQUA);
  text_out("3", 108, 30);
    set_bg_color(COLOR_GREEN);
  set_fg_color(COLOR_WHITE);
  draw_filled_rect_bg(132, 26, 176, 76);
   set_bg_color(COLOR_GREEN);
  text_out("<", 152, 30);
    //Hàng thứ hai: 4, 5, 6, 0 (y: 76 - 76+50)
 set_bg_color(COLOR_GREEN);
  set_fg_color(COLOR_WHITE);
  draw_filled_rect_bg(0, 76, 44, 126);
  set_bg_color(COLOR_GREEN);
  text_out("4", 20, 80);
   set_bg_color(COLOR_AQUA);
  set_fg_color(COLOR_BLACK);
  draw_filled_rect_bg(44, 76, 88, 126);
  set_bg_color(COLOR_AQUA);
  text_out("5", 64, 80);
  set_bg_color(COLOR_GREEN);
  set_fg_color(COLOR_WHITE);
  draw_filled_rect_bg(88, 76, 132, 126);
  set_bg_color(COLOR_GREEN);
  text_out("6", 108, 80);
   set_bg_color(COLOR_AQUA);
  set_fg_color(COLOR_BLACK);
  draw_filled_rect_bg(132, 76, 176, 126);
  set_bg_color(COLOR_AQUA);
  text_out("0", 152, 80);
  //Hàng Thứ 3: 7, 8, 9, > (y: 76 - 76+50)
 set_bg_color(COLOR_AQUA);
  set_fg_color(COLOR_BLACK);
  draw_filled_rect_bg(0, 126, 44, 176);
  set_bg_color(COLOR_AQUA);
  text_out("7", 20, 130);
   set_bg_color(COLOR_GREEN);
  set_fg_color(COLOR_WHITE);
  draw_filled_rect_bg(44, 126, 88, 176);
  set_bg_color(COLOR_GREEN);
  text_out("8", 64, 130);
   set_bg_color(COLOR_AQUA);
  set_fg_color(COLOR_BLACK);
  draw_filled_rect_bg(88, 126, 132, 176);
  set_bg_color(COLOR_AQUA);
  text_out("9", 108, 130);
    set_bg_color(COLOR_GREEN);
  set_fg_color(COLOR_WHITE);
  draw_filled_rect_bg(132, 126, 176, 176);
   set_bg_color(COLOR_GREEN);
  text_out("=", 152, 130);

set_update_period(1, 60000);  
repaint_screen_lines(0, 176);    
}

void convert_1(){
	//struct app_data_** 	app_data_p = get_ptr_temp_buf_2(); 	//	указатель на указатель на данные экрана 
	//struct app_data_ *	app_data = *app_data_p;				//	указатель на данные экрана
struct calend_** 	calend_p = get_ptr_temp_buf_2(); 		//	указатель на указатель на данные экрана 
struct calend_ *	calend = *calend_p;						//	указатель на данные экрана

  //set_backlight_state(1);
	set_bg_color(COLOR_BLACK);
  fill_screen_bg();
  set_fg_color(COLOR_WHITE);
  text_out_center("Chuyển đổi",  88, 5);
  
  long a; // Đổi m sang inch
  a = number * 3937 / 100;
  set_fg_color(COLOR_WHITE);
  char dai_1[10];
	_sprintf(dai_1, "%d", number);
	text_out(dai_1, 5, 30);
	text_out("m", 43, 30);
	text_out("=", 80, 30);
	 char dai_2[10];
	_sprintf(dai_2, "%d", a);
	text_out(dai_2, 90, 30);
	text_out("inch", 139, 30);
	long b; // Đổi inch sang m
  b = number * 254 / 10000;
  set_fg_color(COLOR_WHITE);
  char dai_3[10];
	_sprintf(dai_3, "%d", number);
	text_out(dai_3, 5, 55);
	text_out("inch", 43, 55);
	text_out("=", 80, 55);
	 char dai_4[10];
	_sprintf(dai_4, "%d", b);
	text_out(dai_4, 90, 55);
	text_out("m", 139, 55);
	
	long c; // Đổi m sang feet
  c = number * 328 / 100;
  set_fg_color(COLOR_AQUA);
  char dai_5[10];
	_sprintf(dai_5, "%d", number);
	text_out(dai_5, 5, 80);
	text_out("m", 43, 80);
	text_out("=", 80, 80);
	 char dai_6[10];
	_sprintf(dai_6, "%d", c);
	text_out(dai_6, 90, 80);
	text_out("feet", 139, 80);
	long d; // Đổi feet sang m
  d = number * 3048 / 10000;
  set_fg_color(COLOR_AQUA);
  char dai_7[10];
	_sprintf(dai_7, "%d", number);
	text_out(dai_7, 5, 105);
	text_out("feet", 43, 105);
	text_out("=", 80, 105);
	 char dai_8[10];
	_sprintf(dai_8, "%d", d);
	text_out(dai_8, 90, 105);
	text_out("m", 139, 105);
	
	long e; // Đổi m sang mile
  e = number * 62 / 100000;
  set_fg_color(COLOR_GREEN);
  char dai_9[10];
	_sprintf(dai_9, "%d", number);
	text_out(dai_9, 5, 130);
	text_out("m", 43, 130);
	text_out("=", 80, 130);
	 char dai_10[10];
	_sprintf(dai_10, "%d", e);
	text_out(dai_10, 90, 130);
	text_out("mile", 139, 130);
	long f; // Đổi mile sang m
  f = number * 1609 / 1;
  set_fg_color(COLOR_GREEN);
  char dai_11[10];
	_sprintf(dai_11, "%d", number);
	text_out(dai_11, 5, 155);
	text_out("mile", 43, 155);
	text_out("=", 80, 155);
	char dai_12[10];
	_sprintf(dai_12, "%d", f);
	text_out(dai_12, 90, 155);
	text_out("m", 139, 155);
	

  set_update_period(1, 60000);  
repaint_screen_lines(0, 176);    
}

void convert_2(){
	//struct app_data_** 	app_data_p = get_ptr_temp_buf_2(); 	//	указатель на указатель на данные экрана 
	//struct app_data_ *	app_data = *app_data_p;				//	указатель на данные экрана
struct calend_** 	calend_p = get_ptr_temp_buf_2(); 		//	указатель на указатель на данные экрана 
struct calend_ *	calend = *calend_p;						//	указатель на данные экрана

  //set_backlight_state(1);
	set_bg_color(COLOR_BLACK);
  fill_screen_bg();
  set_fg_color(COLOR_WHITE);
  text_out_center("Trang 2",  88, 5);
  
  long a; // Đổi C sang F
  a = number * 18 / 10 + 32;
  set_fg_color(COLOR_RED);
  char dai_1[10];
	_sprintf(dai_1, "%d", number);
	text_out(dai_1, 5, 30);
	text_out("C", 43, 30);
	text_out("=", 80, 30);
	 char dai_2[10];
	_sprintf(dai_2, "%d", a);
	text_out(dai_2, 90, 30);
	text_out("F", 139, 30);
	long b; // Đổi F sang C
  b = (number - 32) * 10 / 18;
  set_fg_color(COLOR_RED);
  char dai_3[10];
	_sprintf(dai_3, "%d", number);
	text_out(dai_3, 5, 55);
	text_out("F", 43, 55);
	text_out("=", 80, 55);
	 char dai_4[10];
	_sprintf(dai_4, "%d", b);
	text_out(dai_4, 90, 55);
	text_out("C", 139, 55);
	
	long c; // Đổi hPa sang mmHg
  c = number * 75 / 100;
  set_fg_color(COLOR_WHITE);
  char dai_5[10];
	_sprintf(dai_5, "%d", number);
	text_out(dai_5, 5, 80);
	text_out("hPa", 43, 80);
	text_out("=", 80, 80);
	 char dai_6[10];
	_sprintf(dai_6, "%d", c);
	text_out(dai_6, 90, 80);
	text_out("mmHg", 130, 80);
	long d; // Đổi mmHg sang hPa
  d = number * 1333 / 1000;
  set_fg_color(COLOR_WHITE);
  char dai_7[10];
	_sprintf(dai_7, "%d", number);
	text_out(dai_7, 5, 105);
	text_out("mmHg", 34, 105);
	text_out("=", 80, 105);
	 char dai_8[10];
	_sprintf(dai_8, "%d", d);
	text_out(dai_8, 90, 105);
	text_out("hPa", 139, 105);
	
	long e; // Đổi m3 sang ft3
  e = number * 35315 / 1000;
  set_fg_color(COLOR_AQUA);
  char dai_9[10];
	_sprintf(dai_9, "%d", number);
	text_out(dai_9, 5, 130);
	text_out("m3", 43, 130);
	text_out("=", 80, 130);
	 char dai_10[10];
	_sprintf(dai_10, "%d", e);
	text_out(dai_10, 90, 130);
	text_out("feet3", 130, 130);
	long f; // Đổi ft3 sang m3
  f = number * 28 / 1000;
  set_fg_color(COLOR_AQUA);
  char dai_11[10];
	_sprintf(dai_11, "%d", number);
	text_out(dai_11, 5, 155);
	text_out("feet3", 34, 155);
	text_out("=", 80, 155);
	 char dai_12[10];
	_sprintf(dai_12, "%d", f);
	text_out(dai_12, 90, 155);
	text_out("m3", 139, 155);
	

  set_update_period(1, 60000);  
repaint_screen_lines(0, 176);    
}

void convert_3(){
	//struct app_data_** 	app_data_p = get_ptr_temp_buf_2(); 	//	указатель на указатель на данные экрана 
	//struct app_data_ *	app_data = *app_data_p;				//	указатель на данные экрана
struct calend_** 	calend_p = get_ptr_temp_buf_2(); 		//	указатель на указатель на данные экрана 
struct calend_ *	calend = *calend_p;						//	указатель на данные экрана

  //set_backlight_state(1);
	set_bg_color(COLOR_BLACK);
  fill_screen_bg();
  set_fg_color(COLOR_YELLOW);
  text_out_center("Trang 3",  88, 5);
  
  long a; // Đổi Lit sang gallon
  a = number * 264 / 1000;
  set_fg_color(COLOR_AQUA);
  char dai_1[10];
	_sprintf(dai_1, "%d", number);
	text_out(dai_1, 5, 30);
	text_out("L", 43, 30);
	text_out("=", 80, 30);
	 char dai_2[10];
	_sprintf(dai_2, "%d", a);
	text_out(dai_2, 90, 30);
		text_out("Gal", 139, 30);
	long b; // Đổi gallon sang lit
  b = number * 3785 / 1000;
  set_fg_color(COLOR_AQUA);
  char dai_3[10];
	_sprintf(dai_3, "%d", number);
	text_out(dai_3, 5, 55);
	text_out("Gal", 43, 55);
	text_out("=", 80, 55);
	 char dai_4[10];
	_sprintf(dai_4, "%d", b);
	text_out(dai_4, 90, 55);
		text_out("L", 139, 55);
	
	long c; // Đổi kg sang Pounce
  c = number * 2205 / 1000;
  set_fg_color(COLOR_GREEN);
  char dai_5[10];
	_sprintf(dai_5, "%d", number);
	text_out(dai_5, 5, 80);
	text_out("kg", 43, 80);
	text_out("=", 80, 80);
	 char dai_6[10];
	_sprintf(dai_6, "%d", c);
	text_out(dai_6, 90, 80);
		text_out("pound", 128, 80);
	long d; // Đổi Pounce sang kg
  d = number * 45 / 100;
  set_fg_color(COLOR_GREEN);
  char dai_7[10];
	_sprintf(dai_7, "%d", number);
	text_out(dai_7, 5, 105);
	text_out("pound", 32, 105);
	text_out("=", 80, 105);
	 char dai_8[10];
	_sprintf(dai_8, "%d", d);
	text_out(dai_8, 90, 105);
		text_out("kg", 139, 105);
	
	long e; // Đổi m2 sang yard2
  e = number * 1196 / 1000;
  set_fg_color(COLOR_WHITE);
  char dai_9[10];
	_sprintf(dai_9, "%d", number);
	text_out(dai_9, 5, 130);
	text_out("m2", 43, 130);
	text_out("=", 80, 130);
	 char dai_10[10];
	_sprintf(dai_10, "%d", e);
	text_out(dai_10, 90, 130);
		text_out("yard2", 130, 130);
	long f; // Đổi yard2 sang m2
  f = number * 836 / 1000;
  set_fg_color(COLOR_WHITE);
  char dai_11[10];
	_sprintf(dai_11, "%d", number);
	text_out(dai_11, 5, 155);
	text_out("yard2", 34, 155);
	text_out("=", 80, 155);
	 char dai_12[10];
	_sprintf(dai_12, "%d", f);
	text_out(dai_12, 90, 155);
		text_out("m2", 139, 155);
	

  set_update_period(1, 60000);  
repaint_screen_lines(0, 176);    
}

void convert_money(){
	//struct app_data_** 	app_data_p = get_ptr_temp_buf_2(); 	//	указатель на указатель на данные экрана 
	//struct app_data_ *	app_data = *app_data_p;				//	указатель на данные экрана
struct calend_** 	calend_p = get_ptr_temp_buf_2(); 		//	указатель на указатель на данные экрана 
struct calend_ *	calend = *calend_p;						//	указатель на данные экрана

  //set_backlight_state(1);
	set_bg_color(COLOR_BLACK);
  fill_screen_bg();
  set_fg_color(COLOR_RED);
  text_out_center("Money",  88, 5);
  
  long a; // Đổi VND sang EUR
  a = number * 1000 / 25888;
  set_fg_color(COLOR_WHITE);
  char dai_1[10];
	_sprintf(dai_1, "%d", number);
	text_out(dai_1, 5, 30);
	text_out("k VND", 34, 30);
	text_out("=", 80, 30);
	 char dai_2[10];
	_sprintf(dai_2, "%d", a);
	text_out(dai_2, 90, 30);
		text_out("EUR", 139, 30);
	long b; // Đổi EUR sang VND
  b = number * 25888 / 1000;
  set_fg_color(COLOR_WHITE);
  char dai_3[10];
	_sprintf(dai_3, "%d", number);
	text_out(dai_3, 5, 55);
	text_out("EUR", 43, 55);
	text_out("=", 80, 55);
	 char dai_4[10];
	_sprintf(dai_4, "%d", b);
	text_out(dai_4, 90, 55);
		text_out("k VND", 130, 55);
	
	long c; // Đổi VND sang USD
  c = number * 1000 / 22862;
  set_fg_color(COLOR_RED);
  char dai_5[10];
	_sprintf(dai_5, "%d", number);
	text_out(dai_5, 5, 80);
	text_out("k VND", 34, 80);
	text_out("=", 80, 80);
	 char dai_6[10];
	_sprintf(dai_6, "%d", c);
	text_out(dai_6, 90, 80);
		text_out("USD", 139, 80);
	long d; // Đổi USD sang VND
  d = number * 22862 / 1000;
  set_fg_color(COLOR_RED);
  char dai_7[10];
	_sprintf(dai_7, "%d", number);
	text_out(dai_7, 5, 105);
	text_out("USD", 43, 105);
	text_out("=", 80, 105);
	 char dai_8[10];
	_sprintf(dai_8, "%d", d);
	text_out(dai_8, 90, 105);
		text_out("k VND", 130, 105);
	
	long e; // Đổi VND sang AUD
  e = number * 1000 / 17025;
  set_fg_color(COLOR_GREEN);
  char dai_9[10];
	_sprintf(dai_9, "%d", number);
	text_out(dai_9, 5, 130);
	text_out("k VND", 34, 130);
	text_out("=", 80, 130);
	 char dai_10[10];
	_sprintf(dai_10, "%d", e);
	text_out(dai_10, 90, 130);
		text_out("AUD", 139, 130);
	long f; // Đổi AUD sang VND
  f = number * 17025 / 1000;
  set_fg_color(COLOR_GREEN);
  char dai_11[10];
	_sprintf(dai_11, "%d", number);
	text_out(dai_11, 5, 155);
	text_out("AUD", 43, 155);
	text_out("=", 80, 155);
	 char dai_12[10];
	_sprintf(dai_12, "%d", f);
	text_out(dai_12, 90, 155);
		text_out("k VND", 130, 155);
	

  set_update_period(1, 60000);  
repaint_screen_lines(0, 176);    
}

void kiniem(){
	//struct app_data_** 	app_data_p = get_ptr_temp_buf_2(); 	//	указатель на указатель на данные экрана 
	//struct app_data_ *	app_data = *app_data_p;				//	указатель на данные экрана
struct calend_** 	calend_p = get_ptr_temp_buf_2(); 		//	указатель на указатель на данные экрана 
struct calend_ *	calend = *calend_p;						//	указатель на данные экрана

	set_bg_color(COLOR_BLACK);
  fill_screen_bg();
  set_fg_color(COLOR_AQUA);
  text_out_center("Ngày kỉ niệm",  88, 5);
  
  struct datetime_ dt;
		get_current_date_time(&dt);
  
  int month1 = 31;
  int month2 = 31 + 28;
  int month3 = 31 + 28 + 31;
  int month4 = 31 + 28 + 31 + 30;
  int month5 = 31 + 28 + 31 + 30 + 31;
  int month6 = 31 + 28 + 31 + 30 + 31 + 30;
  int month7 = 31 + 28 + 31 + 30 + 31 + 30+ 31;
  int month8 = 31 + 28 + 31 + 30 + 31 + 30+ 31 + 31;
  int month9 = 31 + 28 + 31 + 30 + 31 + 30+ 31 + 31 + 30;
  int month10 = 31 + 28 + 31 + 30 + 31 + 30+ 31 + 31 + 30 + 31;
  int month11 = 31 + 28 + 31 + 30 + 31 + 30+ 31 + 31 + 30 + 31 + 30;
  int month12 = 31 + 28 + 31 + 30 + 31 + 30+ 31 + 31 + 30 + 31 + 30 + 31;
  
  
  int LY; //Vì cái này dành cho các ngày trong tháng 1 và 2 (Tại tháng 2 bị nhuận) còn các tháng còn lại không bị nên không có trừ 1 ở đây
  // If the date is in a January or February of a leap year, you have to subtract one from your total before the final step.
  if ( ( (dt.year + 1) % 4 == 0 ) && ( (dt.year + 1) % 100 != 0 ) ){
	   //LY = -1; Bỏ
	   LY = 366;
   month2 = month2 + 1;
   month3 = month3 + 1;
   month4 = month4 + 1;
   month5 = month5 + 1;
   month6 = month6 + 1;
   month7 = month7 + 1;
   month8 = month8 + 1;
   month9 = month9 + 1;
   month10 = month10 + 1;
   month11 = month11 + 1;
   month12 = month12 + 1;
  } else if ( (dt.year + 1) % 400 == 0 ) {
	   //LY = -1;
	   LY = 366;
	month2 = month2 + 1;
   month3 = month3 + 1;
   month4 = month4 + 1;
   month5 = month5 + 1;
   month6 = month6 + 1;
   month7 = month7 + 1;
   month8 = month8 + 1;
   month9 = month9 + 1;
   month10 = month10 + 1;
   month11 = month11 + 1;
   month12 = month12 + 1;
  } else {
	  LY = 365;
  }
  
  
  int a = 9; //Tháng
  int montha;
  if ( a == 1 ) {
	  montha = 0;
  } else if ( a == 2 ) {
	  montha = month1;
  }else if ( a == 3 ) {
	  montha = month2;
  }else if ( a == 4 ) {
	  montha = month3;
  }else if ( a == 5 ) {
	  montha = month4;
  }else if ( a == 6 ) {
	  montha = month5;
  }else if ( a == 7 ) {
	  montha = month6;
  }else if ( a == 8 ) {
	  montha = month7;
  }else if ( a == 9 ) {
	  montha = month8;
  }else if ( a == 10 ) {
	  montha = month9;
  }else if ( a == 11 ) {
	  montha = month10;
  }else if ( a == 12 ) {
	  montha = month11;
  }
  
  
    int b; // Đổi EUR sang VND
	
	
		if ( dt.month == 1) {
			b = montha - dt.day + 8;
		}
		if ( dt.month == 2) {
			b = montha - month1 - dt.day + 8;
		}
		if ( dt.month == 3) {
			b = montha - month2  - dt.day + 8;
		}
		if ( dt.month == 4) {
			b = montha - month3 - dt.day + 8;
		}
		if ( dt.month == 5) {
			b = montha - month4 - dt.day + 8;
		}
		if ( dt.month == 6) {
			b = montha - month5 - dt.day + 8;
		}
		if ( dt.month == 7) {
			b = montha - month6 - dt.day + 8;
		}
		if ( dt.month == 8) {
			b = montha - month7 - dt.day + 8;
		}
		if ( dt.month == 9) {
			
			b = montha - month8 - dt.day + 8;
		
		}
		if ( dt.month == 10) {
			b = montha - month9 - dt.day + 8;
		}
		if ( dt.month == 11) {
			b = montha - month10 - dt.day + 8;
		}
		if ( dt.month == 12) {
			b = montha - month11 - dt.day + 8;
		}
		if ( b < 0 ) {
			b = b + LY;
		}
		if ((dt.day == 9 - 0 ) && (dt.month == a ) ){
		set_fg_color(COLOR_YELLOW);	
		} else {
		set_fg_color(COLOR_GREEN);
		}
	text_out("SN Dợ", 5, 30);
	text_out("8/9/1999", 100, 30);
	text_out("Còn", 5, 55);
	 char dai_2[10];
	_sprintf(dai_2, "%d", b);
	text_out(dai_2, 80, 55);
		text_out("Day", 139, 55);
		/* Thử nghiệm hàm mới tạo thông báo cho đồng hồ, Dựa vào file Libbip.h
		if ((dt.day == 16 ) && (dt.month == 4 ) ){
		create_and_show_notification(31, "Hello", "Xin chào, đây là test", "Bin");	
		}
		Cấu trúc của hàm này có 2 hàm:
		// Chức năng tạo thông báo (thông báo bật lên)
extern int add_notification(int notif_type, int timestamp, char *title, char *msg, char *app_name);	// tạo và thêm thông báo mới vào danh sách thông báo
extern int create_and_show_notification(int notif_type, char *title, char *msg, char *app_name);	// tạo và hiển thị thông báo đã tạo (vẫn còn trong danh sách)
Trong đó, cả 2 hàm đều có thể tự tạo thông báo trên đồng hồ luôn và hiển thị trong mục thông báo của đồng hồ, cả 2 hàm đều có thông sô giống nhau được Bin giải nghĩa như sau:
Biến int notif_type: Biến này dùng để hiển thị kí hiệu của app cần thông báo, Mình có thể tham khảo các app trong dòng 416 trong file libbip.h (Ví dụ: Muốn hiển thị kí hiệu cho app Facebook thì là chọn số 8)
Biến char *title: Ghi Tiêu đề bình thường trong 2 dấu nháy " "
Biến char *msg: Ghi nội dung thông báo bình thường trong 2 dấu nháy " "
char *app_name: Ghi tên app, tùy ý mình, ghi tên gì cũng được cũng trong 2 dấu nháy " "
		*/
		
		/*if ((dt.day == 9 - 5 ) && (dt.month == a ) ){
		create_and_show_notification(27, "Sinh nhật Dợ", "Còn 5 ngày là đến sinh nhật dợ: 8/9", "Bin");	
		}
		if ((dt.day == 9 - 4 ) && (dt.month == a ) ){
		create_and_show_notification(27, "Sinh nhật Dợ", "Còn 4 ngày là đến sinh nhật dợ: 8/9", "Bin");	
		}
		if ((dt.day == 9 - 3 ) && (dt.month == a ) ){
		create_and_show_notification(27, "Sinh nhật Dợ", "Còn 3 ngày là đến sinh nhật dợ: 8/9", "Bin");	
		}
		if ((dt.day == 9 - 2 ) && (dt.month == a ) ){
		create_and_show_notification(27, "Sinh nhật Dợ", "Còn 2 ngày là đến sinh nhật dợ: 8/9", "Bin");	
		}*/
		if ((dt.day == 9 - 1 ) && (dt.month == a ) ){
		create_and_show_notification(27, "Sinh nhật Dợ", "Còn 1 ngày là đến sinh nhật dợ: 8/9", "Bin");	
		}
		if ((dt.day == 9 - 0 ) && (dt.month == a ) ){
		create_and_show_notification(27, "Sinh nhật Dợ", "Chúc mừng sinh nhật Dợ, hôm nay: 8/9", "Bin");	
		}
		
		
// -------- Sinh nhật Dợ ------------

  int d;
  int c = 6; //Tháng
  int y = 2020; //Năm
  int monthc;
  if ( c == 1 ) {
	  monthc = 0;
  } else if ( c == 2 ) {
	  monthc = month1;
  }else if ( c == 3 ) {
	  monthc = month2;
  }else if ( c == 4 ) {
	  monthc = month3;
  }else if ( c == 5 ) {
	  monthc = month4;
  }else if ( c == 6 ) {
	  monthc = month5;
  }else if ( c == 7 ) {
	  monthc = month6;
  }else if ( c == 8 ) {
	  monthc = month7;
  }else if ( c == 9 ) {
	  monthc = month8;
  }else if ( c == 10 ) {
	  monthc = month9;
  }else if ( c == 11 ) {
	  monthc = month10;
  }else if ( c == 12 ) {
	  monthc = month11;
  }
  
	int sum = 0;
	if ( dt.month == 1) {
			d = monthc - dt.day + 7;
		}
		if ( dt.month == 2) {
			d = monthc - month1 - dt.day + 7;
		}
		if ( dt.month == 3) {
			d = monthc - month2  - dt.day + 7;
		}
		if ( dt.month == 4) {
			d = monthc - month3 - dt.day + 7;
		}
		if ( dt.month == 5) {
			d = monthc - month4 - dt.day + 7;
		}
		if ( dt.month == 6) {
			d = monthc - month5 - dt.day + 7;
		}
		if ( dt.month == 7) {
			d = monthc - month6 - dt.day + 7;
		}
		if ( dt.month == 7) {
			d = monthc - month7 - dt.day + 7;
		}
		if ( dt.month == 9) {
			
			d = monthc - month8 - dt.day + 7;
		
		}
		if ( dt.month == 10) {
			d = monthc - month9 - dt.day + 7;
		}
		if ( dt.month == 11) {
			d = monthc - month10 - dt.day + 7;
		}
		if ( dt.month == 12) {
			d = monthc - month11 - dt.day + 7;
		}
	
	for (int i = 2020 ; i < dt.year ; i++){
		 if ( ( i % 4 == 0 ) && ( i % 100 != 0 ) ){
			   //LY = -1; Bỏ
			   sum = sum + 366;
		  } else if ( i % 400 == 0 ) {
			   //LY = -1;
			   sum = sum + 366;
		  } else {
			  sum = sum + 365;
		  }
	}
	if ( c >= 2 ) {
		if ( ( y % 4 == 0 ) && ( y % 100 != 0 ) ){
			   //LY = -1; Bỏ
			   sum = sum - 1;
		  } else if ( y % 400 == 0 ) {
			   //LY = -1;
			   sum = sum - 1;
		  } else {
			  sum = sum;
		  }
	}
	d = sum - d ;
	if ((dt.day == 7 - 0 ) && (dt.month == c ) ){
	 set_fg_color(COLOR_YELLOW);
	} else {
		 set_fg_color(COLOR_RED);
	}
	text_out("Hẹn Hò", 5, 80);
	text_out("7/6/2020", 100, 80);
	text_out("WithU", 5, 105);
	 char dai_6[10];
	_sprintf(dai_6, "%d", d);
	text_out(dai_6, 80, 105);
		text_out("Day", 139, 105);
		
		/*if ((dt.day == 7 - 5 ) && (dt.month == c ) ){
		create_and_show_notification(18, "Kỉ niệm ngày quen nhau", "Còn 5 ngày là đến ngày kỉ niệm: 7/6", "Bin");	
		}
		if ((dt.day == 7 - 4 ) && (dt.month == c ) ){
		create_and_show_notification(18, "Kỉ niệm ngày quen nhau", "Còn 4 ngày là đến ngày kỉ niệm: 7/6", "Bin");	
		}
		if ((dt.day == 7 - 3 ) && (dt.month == c ) ){
		create_and_show_notification(18, "Kỉ niệm ngày quen nhau", "Còn 3 ngày là đến ngày kỉ niệm: 7/6", "Bin");	
		}
		if ((dt.day == 7 - 2 ) && (dt.month == c ) ){
		create_and_show_notification(18, "Kỉ niệm ngày quen nhau", "Còn 2 ngày là đến ngày kỉ niệm: 7/6", "Bin");	
		}
		if ((dt.day == 7 - 1 ) && (dt.month == c ) ){
		create_and_show_notification(18, "Kỉ niệm ngày quen nhau", "Còn 1 ngày là đến ngày kỉ niệm: 7/6", "Bin");	
		}*/
		if ((dt.day == 7 - 0 ) && (dt.month == c ) ){
		create_and_show_notification(18, "Kỉ niệm ngày quen nhau", "Chúc mừng Ngày kỉ niệm, hôm nay: 7/6", "Bin");	
		}
// -------- Hẹn hò ------------

  
    int e; // Đổi EUR sang VND
	int kinh1;
	int f = 8; // Ngày có kinh vào tháng 4
	int g; //Ngày có kinh tháng tiếp theo
	int m; //Tháng để hiển thị ngày có kinh
	int h; //Số ngày hiện tại đến ngày có kinh gần nhất
	
		if ( dt.month == 1) {
			h = f - dt.day;
			if ( h < 0 ) {
			h = h + 30;
			g = f + 30 - 31;
			m = dt.month + 1;
			} else {
			g = f;
			m = dt.month;	
			}
		}
		if ( dt.month == 2) {
			h = f - dt.day;
			if ( h < 0 ) {
			h = h + 30;
			m = dt.month + 1;
				if ( ( dt.year % 4 == 0 ) && ( dt.year % 100 != 0 ) ){
				   g = f + 30 - 29;
			  } else if ( dt.year % 400 == 0 ) {
				g = f + 30 - 29;
			  } else {
				 g = f + 30 - 28;
			  }
			} else {
			g = f;
			m = dt.month;	
			}
		}
		if ( dt.month == 3) {
			h = f - dt.day;
			if ( h < 0 ) {
			h = h + 30;
			g = f + 30 - 31;
			m = dt.month + 1;
			} else {
			g = f;
			m = dt.month;	
			}
		}
		if ( dt.month == 4) {
			h = f - dt.day;
			if ( h < 0 ) {
			h = h + 30;
			g = f + 30 - 30;
			m = dt.month + 1;
			} else {
			g = f;
			m = dt.month;	
			}
		}
		if ( dt.month == 5) {
			h = f - dt.day;
			if ( h < 0 ) {
			h = h + 30;
			g = f + 30 - 31;
			m = dt.month + 1;
			} else {
			g = f;
			m = dt.month;	
			}
		}
		if ( dt.month == 6) {
			h = f - dt.day;
			if ( h < 0 ) {
			h = h + 30;
			g = f + 30 - 30;
			m = dt.month + 1;
			} else {
			g = f;
			m = dt.month;	
			}
		}
		if ( dt.month == 7) {
			h = f - dt.day;
			if ( h < 0 ) {
			h = h + 30;
			g = f + 30 - 31;
			m = dt.month + 1;
			} else {
			g = f;
			m = dt.month;	
			}
		}
		if ( dt.month == 8) {
			h = f - dt.day;
			if ( h < 0 ) {
			h = h + 30;
			g = f + 30 - 31;
			m = dt.month + 1;
			} else {
			g = f;
			m = dt.month;	
			}
		}
		if ( dt.month == 9) {
			
			h = f - dt.day;
			if ( h < 0 ) {
			h = h + 30;
			g = f + 30 - 30;
			m = dt.month + 1;
			} else {
			g = f;
			m = dt.month;	
			}
		
		}
		if ( dt.month == 10) {
			h = f - dt.day;
			if ( h < 0 ) {
			h = h + 30;
			g = f + 30 - 31;
			m = dt.month + 1;
			} else {
			g = f;
			m = dt.month;	
			}
		}
		if ( dt.month == 11) {
			h = f - dt.day;
			if ( h < 0 ) {
			h = h + 30;
			g = f + 30 - 30;
			m = dt.month + 1;
			} else {
			g = f;
			m = dt.month;	
			}
		}
		if ( dt.month == 12) {
			h = f - dt.day;
			if ( h < 0 ) {
			h = h + 30;
			g = f + 30 - 31;
			m = dt.month + 1;
			} else {
			g = f;
			m = dt.month;	
			}
		}
		
		if ( (h < 30 ) && ( h > 26 ) ) {
		set_fg_color(COLOR_BLUE);
		} else if ( (h < 22 ) && ( h > 15 ) ) {
			set_fg_color(COLOR_PURPLE);
		} else if ( (h < 15 ) && ( h > 11 ) ) {
			set_fg_color(COLOR_PURPLE);
		} else if ( h == 15 ) {
			set_fg_color(COLOR_YELLOW);
		} else {
	set_fg_color(COLOR_GREEN);
		}
		text_out("Kinh nguyệt", 5, 130);
		 char dai_3[12];
		 if (m > 12) {
			m = m - 1;
		 _sprintf(dai_3, "%02d/%02d/%02d", g , m, dt.year + 1 - 2000);
		 } else {
		_sprintf(dai_3, "%02d/%02d/%02d", g , m, dt.year - 2000);	 
		 }
	text_out(dai_3, 100, 130);
	text_out("Còn", 5, 155);
	 char dai_4[10];
	_sprintf(dai_4, "%d", h);
	text_out(dai_4, 80, 155);
		text_out("Day", 139, 155);
		
		/*Lưu ý về hàm Char này:
		Hàm có công thức: char x[y];
		Thì trong đó x là tên biến, còn y là số kí tự mà x sẽ được gán vào. 
		Y càng lớn thì càng hiển thị được nhiều như ở dưới đây
		*/
		
		/*
		if ( h == 14 ) {
			char dai_8[128];
		 _sprintf(dai_8, "%02s %02d/%02d/%02d", "Ngày mai là ngày thụ thai cao, ngày có kinh dự đoán:" , g , m, dt.year - 2000 );
		create_and_show_notification(7, "Ngày thụ thai", dai_8, "Bin");	
		}*/
		if ( h == 15 ) {
			char dai_9[128];
		 _sprintf(dai_9, "%02s %02d/%02d/%02d", "Hôm nay là ngày thụ thai cao, ngày có kinh dự đoán:" , g , m, dt.year - 2000 );
		create_and_show_notification(7, "Ngày thụ thai", dai_9, "Bin");	
		}/*
		if ( h == 16 ) {
			char dai_10[128];
		 _sprintf(dai_10, "%02s %02d/%02d/%02d", "Hôm qua là ngày thụ thai cao, ngày có kinh dự đoán:" , g , m, dt.year - 2000 );
		create_and_show_notification(7, "Ngày thụ thai", dai_10, "Bin");	
		}
		if ( h == 2 ) {
			char dai_11[128];
		 _sprintf(dai_11, "%02s  %02d/%02d/%02d", "Còn 2 ngày là đến ngày hành kinh, ngày có kinh dự đoán:" , g , m, dt.year - 2000 );
		create_and_show_notification(7, "Ngày hành kinh", dai_11, "Bin");	
		}
		if ( h == 1 ) {
			char dai_12[128];
		 _sprintf(dai_12, "%02s %02d/%02d/%02d", "Còn 1 ngày là đến ngày hành kinh, ngày có kinh dự đoán:" , g , m, dt.year - 2000 );
		create_and_show_notification(7, "Ngày hành kinh", dai_12, "Bin");	
		}*/
		if ( h == 0 ) {
			char dai_13[128];
		 _sprintf(dai_13, "%02s %02d/%02d/%02d", "Hôm nay là ngày hành kinh thứ 1, ngày có kinh dự đoán:" , g , m, dt.year - 2000 );
		create_and_show_notification(7, "Ngày hành kinh", dai_13, "Bin");	
		}
		/*if ( h == 29 ) {
			char dai_14[128];
		 _sprintf(dai_14, "%02s %02d/%02d/%02d", "Hôm nay là ngày hành kinh thứ 2, ngày có kinh tiếp theo dự đoán:" , g , m, dt.year - 2000 );
		create_and_show_notification(7, "Ngày hành kinh", dai_14, "Bin");	
		}/*
		if (( h == 28 ) ) {
			char dai_15[128];
		 _sprintf(dai_15, "%02s: %02d/%02d/%02d", "Hôm nay là ngày hành kinh thứ 3, ngày có kinh tiếp theo dự đoán:" , g , m, dt.year - 2000 );
		create_and_show_notification(7, "Ngày hành kinh", dai_15, "Bin");	
		}/*
		if ( h == 27 ) {
			char dai_16[128];
		 _sprintf(dai_16, "%02s  %02d/%02d/%02d", "Hôm nay là ngày hành kinh cuối cùng, ngày có kinh tiếp theo dự đoán:" , g , m, dt.year - 2000 );
		create_and_show_notification(7, "Ngày hành kinh", dai_16, "Bin");	
		}*/
		

  set_update_period(1, 600000);  
repaint_screen_lines(0, 176);    
}

/*
void kiniem1(){
	//struct app_data_** 	app_data_p = get_ptr_temp_buf_2(); 	//	указатель на указатель на данные экрана 
	//struct app_data_ *	app_data = *app_data_p;				//	указатель на данные экрана
struct calend_** 	calend_p = get_ptr_temp_buf_2(); 		//	указатель на указатель на данные экрана 
struct calend_ *	calend = *calend_p;						//	указатель на данные экрана

	set_bg_color(COLOR_BLACK);
  fill_screen_bg();
  set_fg_color(COLOR_AQUA);
  text_out_center("Ngày kỉ niệm",  88, 5);
  
  struct datetime_ dt;
		get_current_date_time(&dt);
  
  int month1 = 31;
  int month2 = 31 + 28;
  int month3 = 31 + 28 + 31;
  int month4 = 31 + 28 + 31 + 30;
  int month5 = 31 + 28 + 31 + 30 + 31;
  int month6 = 31 + 28 + 31 + 30 + 31 + 30;
  int month7 = 31 + 28 + 31 + 30 + 31 + 30+ 31;
  int month8 = 31 + 28 + 31 + 30 + 31 + 30+ 31 + 31;
  int month9 = 31 + 28 + 31 + 30 + 31 + 30+ 31 + 31 + 30;
  int month10 = 31 + 28 + 31 + 30 + 31 + 30+ 31 + 31 + 30 + 31;
  int month11 = 31 + 28 + 31 + 30 + 31 + 30+ 31 + 31 + 30 + 31 + 30;
  int month12 = 31 + 28 + 31 + 30 + 31 + 30+ 31 + 31 + 30 + 31 + 30 + 31;
  
  
  int LY; //Vì cái này dành cho các ngày trong tháng 1 và 2 (Tại tháng 2 bị nhuận) còn các tháng còn lại không bị nên không có trừ 1 ở đây
  // If the date is in a January or February of a leap year, you have to subtract one from your total before the final step.
  if ( ( (dt.year + 1) % 4 == 0 ) && ( (dt.year + 1) % 100 != 0 ) ){
	   //LY = -1; Bỏ
	   LY = 366;
   month2 = month2 + 1;
   month3 = month3 + 1;
   month4 = month4 + 1;
   month5 = month5 + 1;
   month6 = month6 + 1;
   month7 = month7 + 1;
   month8 = month8 + 1;
   month9 = month9 + 1;
   month10 = month10 + 1;
   month11 = month11 + 1;
   month12 = month12 + 1;
  } else if ( (dt.year + 1) % 400 == 0 ) {
	   //LY = -1;
	   LY = 366;
	month2 = month2 + 1;
   month3 = month3 + 1;
   month4 = month4 + 1;
   month5 = month5 + 1;
   month6 = month6 + 1;
   month7 = month7 + 1;
   month8 = month8 + 1;
   month9 = month9 + 1;
   month10 = month10 + 1;
   month11 = month11 + 1;
   month12 = month12 + 1;
  } else {
	  LY = 365;
  }
  
  
  int a = 10; //Tháng
  int montha;
  if ( a == 1 ) {
	  montha = 0;
  } else if ( a == 2 ) {
	  montha = month1;
  }else if ( a == 3 ) {
	  montha = month2;
  }else if ( a == 4 ) {
	  montha = month3;
  }else if ( a == 5 ) {
	  montha = month4;
  }else if ( a == 6 ) {
	  montha = month5;
  }else if ( a == 7 ) {
	  montha = month6;
  }else if ( a == 8 ) {
	  montha = month7;
  }else if ( a == 9 ) {
	  montha = month8;
  }else if ( a == 10 ) {
	  montha = month9;
  }else if ( a == 11 ) {
	  montha = month10;
  }else if ( a == 12 ) {
	  montha = month11;
  }
  
  
    int b; // Đổi EUR sang VND
	set_fg_color(COLOR_YELLOW);
	text_out("SN Ba", 5, 30);
	text_out("10/1/1969", 100, 30);
	text_out("Còn", 5, 55);
	
		if ( dt.month == 1) {
			b = montha - dt.day + 1;
		}
		if ( dt.month == 2) {
			b = montha - month1 - dt.day + 1;
		}
		if ( dt.month == 3) {
			b = montha - month2  - dt.day + 1;
		}
		if ( dt.month == 4) {
			b = montha - month3 - dt.day + 1;
		}
		if ( dt.month == 5) {
			b = montha - month4 - dt.day + 1;
		}
		if ( dt.month == 6) {
			b = montha - month5 - dt.day + 1;
		}
		if ( dt.month == 7) {
			b = montha - month6 - dt.day + 1;
		}
		if ( dt.month == 8) {
			b = montha - month7 - dt.day + 1;
		}
		if ( dt.month == 9) {
			
			b = montha - month8 - dt.day + 1;
		
		}
		if ( dt.month == 10) {
			b = montha - month9 - dt.day + 1;
		}
		if ( dt.month == 11) {
			b = montha - month10 - dt.day + 1;
		}
		if ( dt.month == 12) {
			b = montha - month11 - dt.day + 1;
		}
		if ( b < 0 ) {
			b = b + LY;
		}
		 if ( ( (dt.year + 1) % 4 == 0 ) && ( (dt.year + 1) % 100 != 0 ) ){
				   if (a <= 2 ) {
					   b = b - 1;
				   } else {
					  b = b; 
				   }
		} else if ( (dt.year + 1) % 400 == 0 ) {
			   if (a <= 2 ) {
				   b = b - 1;
			   } else {
				  b = b; 
			   }
		} else {
			  if (a <= 2 ) {
				   b = b + 1;
			   } else {
				  b = b; 
			   }
		}
	 char dai_2[10];
	_sprintf(dai_2, "%d", b);
	text_out(dai_2, 80, 55);
		text_out("Day", 139, 55);
		
		if ((dt.day == 10 - 5 ) && (dt.month == a ) ){
		create_and_show_notification(27, "Sinh nhật Ba", "Còn 5 ngày là đến sinh nhật Ba: 10/1", "Bin");	
		}
		if ((dt.day == 10 - 4 ) && (dt.month == a ) ){
		create_and_show_notification(27, "Sinh nhật Ba", "Còn 4 ngày là đến sinh nhật Ba: 10/1", "Bin");	
		}
		if ((dt.day == 10 - 3 ) && (dt.month == a ) ){
		create_and_show_notification(27, "Sinh nhật Ba", "Còn 3 ngày là đến sinh nhật Ba: 10/1", "Bin");	
		}
		if ((dt.day == 10 - 2 ) && (dt.month == a ) ){
		create_and_show_notification(27, "Sinh nhật Ba", "Còn 2 ngày là đến sinh nhật Ba: 10/1", "Bin");	
		}
		if ((dt.day == 10 - 1 ) && (dt.month == a ) ){
		create_and_show_notification(27, "Sinh nhật Ba", "Còn 1 ngày là đến sinh nhật Ba: 10/1", "Bin");	
		}
		if ((dt.day == 10 - 0 ) && (dt.month == a ) ){
		create_and_show_notification(27, "Sinh nhật Ba", "Chúc mừng sinh nhật Ba, hôm nay: 10/1", "Bin");	
		}
// -------- Sinh nhật Ba ------------

   int c = 3; //Tháng
  int monthc;
  if ( c == 1 ) {
	  monthc = 0;
  } else if ( c == 2 ) {
	  monthc = month1;
  }else if ( c == 3 ) {
	  monthc = month2;
  }else if ( c == 4 ) {
	  monthc = month3;
  }else if ( c == 5 ) {
	  monthc = month4;
  }else if ( c == 6 ) {
	  monthc = month5;
  }else if ( c == 7 ) {
	  monthc = month6;
  }else if ( c == 8 ) {
	  monthc = month7;
  }else if ( c == 9 ) {
	  monthc = month8;
  }else if ( c == 10 ) {
	  monthc = month9;
  }else if ( c == 11 ) {
	  monthc = month10;
  }else if ( c == 12 ) {
	  monthc = month11;
  }
  
  
    int d; 
	set_fg_color(COLOR_BLUE);
	text_out("SN Mẹ", 5, 80);
	text_out("26/3/1973", 100, 80);
	text_out("Còn", 5, 105);
	
		if ( dt.month == 1) {
			d = monthc - dt.day + 26;
		}
		if ( dt.month == 2) {
			d = monthc - month1 - dt.day + 26;
		}
		if ( dt.month == 3) {
			d = monthc - month2  - dt.day + 26;
		}
		if ( dt.month == 4) {
			d = monthc - month3 - dt.day + 26;
		}
		if ( dt.month == 5) {
			d = monthc - month4 - dt.day + 26;
		}
		if ( dt.month == 6) {
			d = monthc - month5 - dt.day + 26;
		}
		if ( dt.month == 7) {
			d = monthc - month6 - dt.day + 26;
		}
		if ( dt.month == 8) {
			d = monthc - month7 - dt.day + 26;
		}
		if ( dt.month == 9) {
			
			d = monthc - month8 - dt.day + 26;
		
		}
		if ( dt.month == 10) {
			d = monthc - month9 - dt.day + 26;
		}
		if ( dt.month == 11) {
			d = monthc - month10 - dt.day + 26;
		}
		if ( dt.month == 12) {
			d = monthc - month11 - dt.day + 26;
		}
		if ( d < 0 ) {
			d = d + LY;
		}
	 char dai_1[10];
	_sprintf(dai_1, "%d", d);
	text_out(dai_1, 80, 105);
		text_out("Day", 139, 105);
		
		if ((dt.day == 26 - 5 ) && (dt.month == c ) ){
		create_and_show_notification(27, "Sinh nhật Mẹ", "Còn 5 ngày là đến sinh nhật Mẹ: 26/3", "Bin");	
		}
		if ((dt.day == 26 - 4 ) && (dt.month == c ) ){
		create_and_show_notification(27, "Sinh nhật Mẹ", "Còn 4 ngày là đến sinh nhật Mẹ: 26/3", "Bin");	
		}
		if ((dt.day == 26 - 3 ) && (dt.month == c ) ){
		create_and_show_notification(27, "Sinh nhật Mẹ", "Còn 3 ngày là đến sinh nhật Mẹ: 26/3", "Bin");	
		}
		if ((dt.day == 26 - 2 ) && (dt.month == c ) ){
		create_and_show_notification(27, "Sinh nhật Mẹ", "Còn 2 ngày là đến sinh nhật Mẹ: 26/3", "Bin");	
		}
		if ((dt.day == 26 - 1 ) && (dt.month == c ) ){
		create_and_show_notification(27, "Sinh nhật Mẹ", "Còn 1 ngày là đến sinh nhật Mẹ: 26/3", "Bin");	
		}
		if ((dt.day == 26 - 0 ) && (dt.month == c ) ){
		create_and_show_notification(27, "Sinh nhật Mẹ", "Chúc mừng sinh nhật Mẹ, hôm nay: 26/3", "Bin");	
		}
// -------- Sinh nhật Mẹ ------------

   int e = 6; //Tháng
  int monthe;
  if ( e == 1 ) {
	  monthe = 0;
  } else if ( e == 2 ) {
	  monthe = month1;
  }else if ( e == 3 ) {
	  monthe = month2;
  }else if ( e == 4 ) {
	  monthe = month3;
  }else if ( e == 5 ) {
	  monthe = month4;
  }else if ( e == 6 ) {
	  monthe = month5;
  }else if ( e == 7 ) {
	  monthe = month6;
  }else if ( e == 8 ) {
	  monthe = month7;
  }else if ( e == 9 ) {
	  monthe = month8;
  }else if ( e == 10 ) {
	  monthe = month9;
  }else if ( e == 11 ) {
	  monthe = month10;
  }else if ( e == 12 ) {
	  monthe = month11;
  }
  
  
    int f; 
	set_fg_color(COLOR_PURPLE);
	text_out("SN My", 5, 130);
	text_out("7/6/2022", 100, 130);
	text_out("Còn", 5, 155);
	
		if ( dt.month == 1) {
			f = monthe - dt.day + 7;
		}
		if ( dt.month == 2) {
			f = monthe - month1 - dt.day + 7;
		}
		if ( dt.month == 3) {
			f = monthe - month2  - dt.day + 7;
		}
		if ( dt.month == 4) {
			f = monthe - month3 - dt.day + 7;
		}
		if ( dt.month == 5) {
			f = monthe - month4 - dt.day + 7;
		}
		if ( dt.month == 6) {
			f = monthe - month5 - dt.day + 7;
		}
		if ( dt.month == 7) {
			f = monthe - month6 - dt.day + 7;
		}
		if ( dt.month == 8) {
			f = monthe - month7 - dt.day + 7;
		}
		if ( dt.month == 9) {
			
			f = monthe - month8 - dt.day + 7;
		
		}
		if ( dt.month == 10) {
			f = monthe - month9 - dt.day + 7;
		}
		if ( dt.month == 11) {
			f = monthe - month10 - dt.day + 7;
		}
		if ( dt.month == 12) {
			f = monthe - month11 - dt.day + 7;
		}
		if ( f < 0 ) {
			f = f + LY;
		}
	 char dai_3[10];
	_sprintf(dai_3, "%d", f);
	text_out(dai_3, 80, 155);
		text_out("Day", 139, 155);
		
		if ((dt.day == 7 - 5 ) && (dt.month == e ) ){
		create_and_show_notification(27, "Sinh nhật My", "Còn 5 ngày là đến sinh nhật My: 7/6", "Bin");	
		}
		if ((dt.day == 7 - 4 ) && (dt.month == e ) ){
		create_and_show_notification(27, "Sinh nhật My", "Còn 4 ngày là đến sinh nhật My: 7/6", "Bin");	
		}
		if ((dt.day == 7 - 3 ) && (dt.month == e ) ){
		create_and_show_notification(27, "Sinh nhật My", "Còn 3 ngày là đến sinh nhật My: 7/6", "Bin");	
		}
		if ((dt.day == 7 - 2 ) && (dt.month == e ) ){
		create_and_show_notification(27, "Sinh nhật My", "Còn 2 ngày là đến sinh nhật My: 7/6", "Bin");	
		}
		if ((dt.day == 7 - 1 ) && (dt.month == e ) ){
		create_and_show_notification(27, "Sinh nhật My", "Còn 1 ngày là đến sinh nhật My: 7/6", "Bin");	
		}
		if ((dt.day == 7 - 0 ) && (dt.month == e ) ){
		create_and_show_notification(27, "Sinh nhật My", "Chúc mừng sinh nhật My, hôm nay: 7/6", "Bin");	
		}
// -------- Sinh nhật My ------------

  set_update_period(1, 600000);  
repaint_screen_lines(0, 176);    
}


void kiniem2(){
	//struct app_data_** 	app_data_p = get_ptr_temp_buf_2(); 	//	указатель на указатель на данные экрана 
	//struct app_data_ *	app_data = *app_data_p;				//	указатель на данные экрана
struct calend_** 	calend_p = get_ptr_temp_buf_2(); 		//	указатель на указатель на данные экрана 
struct calend_ *	calend = *calend_p;						//	указатель на данные экрана

	set_bg_color(COLOR_BLACK);
  fill_screen_bg();
  set_fg_color(COLOR_WHITE);
  text_out_center("Tính ngày",  88, 5);
  
  struct datetime_ dt;
		get_current_date_time(&dt);
  
  int month1 = 31;
  int month2 = 31 + 28;
  int month3 = 31 + 28 + 31;
  int month4 = 31 + 28 + 31 + 30;
  int month5 = 31 + 28 + 31 + 30 + 31;
  int month6 = 31 + 28 + 31 + 30 + 31 + 30;
  int month7 = 31 + 28 + 31 + 30 + 31 + 30+ 31;
  int month8 = 31 + 28 + 31 + 30 + 31 + 30+ 31 + 31;
  int month9 = 31 + 28 + 31 + 30 + 31 + 30+ 31 + 31 + 30;
  int month10 = 31 + 28 + 31 + 30 + 31 + 30+ 31 + 31 + 30 + 31;
  int month11 = 31 + 28 + 31 + 30 + 31 + 30+ 31 + 31 + 30 + 31 + 30;
  int month12 = 31 + 28 + 31 + 30 + 31 + 30+ 31 + 31 + 30 + 31 + 30 + 31;
  
  
  int LY; //Vì cái này dành cho các ngày trong tháng 1 và 2 (Tại tháng 2 bị nhuận) còn các tháng còn lại không bị nên không có trừ 1 ở đây
  // If the date is in a January or February of a leap year, you have to subtract one from your total before the final step.
  if ( ( (dt.year + 1) % 4 == 0 ) && ( (dt.year + 1) % 100 != 0 ) ){
	   //LY = -1; Bỏ
	   LY = 366;
   month2 = month2 + 1;
   month3 = month3 + 1;
   month4 = month4 + 1;
   month5 = month5 + 1;
   month6 = month6 + 1;
   month7 = month7 + 1;
   month8 = month8 + 1;
   month9 = month9 + 1;
   month10 = month10 + 1;
   month11 = month11 + 1;
   month12 = month12 + 1;
  } else if ( (dt.year + 1) % 400 == 0 ) {
	   //LY = -1;
	   LY = 366;
	month2 = month2 + 1;
   month3 = month3 + 1;
   month4 = month4 + 1;
   month5 = month5 + 1;
   month6 = month6 + 1;
   month7 = month7 + 1;
   month8 = month8 + 1;
   month9 = month9 + 1;
   month10 = month10 + 1;
   month11 = month11 + 1;
   month12 = month12 + 1;
  } else {
	  LY = 365;
  }
  
  int a_day = number_time / 1000000 ;
  int a = number_time / 10000 - a_day * 100;
  int a_year = number_time - a * 10000 - a_day * 1000000;
  int montha;
  if ( a == 1 ) {
	  montha = 0;
  } else if ( a == 2 ) {
	  montha = month1;
  }else if ( a == 3 ) {
	  montha = month2;
  }else if ( a == 4 ) {
	  montha = month3;
  }else if ( a == 5 ) {
	  montha = month4;
  }else if ( a == 6 ) {
	  montha = month5;
  }else if ( a == 7 ) {
	  montha = month6;
  }else if ( a == 8 ) {
	  montha = month7;
  }else if ( a == 9 ) {
	  montha = month8;
  }else if ( a == 10 ) {
	  montha = month9;
  }else if ( a == 11 ) {
	  montha = month10;
  }else if ( a == 12 ) {
	  montha = month11;
  }
  
  
    int b; // Đổi EUR sang VND
	set_fg_color(COLOR_PURPLE);
	text_out("Ngày", 5, 30);
	char dai_1[12];
	_sprintf(dai_1, "%02d/%02d/%02d", a_day , a, a_year);
	text_out(dai_1, 100, 30);
	text_out("Còn", 5, 55);
	
		if ( dt.month == 1) {
			b = montha - dt.day + a_day;
		}
		if ( dt.month == 2) {
			b = montha - month1 - dt.day + a_day;
		}
		if ( dt.month == 3) {
			b = montha - month2  - dt.day + a_day;
		}
		if ( dt.month == 4) {
			b = montha - month3 - dt.day + a_day;
		}
		if ( dt.month == 5) {
			b = montha - month4 - dt.day + a_day;
		}
		if ( dt.month == 6) {
			b = montha - month5 - dt.day + a_day;
		}
		if ( dt.month == 7) {
			b = montha - month6 - dt.day + a_day;
		}
		if ( dt.month == 8) {
			b = montha - month7 - dt.day + a_day;
		}
		if ( dt.month == 9) {
			
			b = montha - month8 - dt.day + a_day;
		
		}
		if ( dt.month == 10) {
			b = montha - month9 - dt.day + a_day;
		}
		if ( dt.month == 11) {
			b = montha - month10 - dt.day + a_day;
		}
		if ( dt.month == 12) {
			b = montha - month11 - dt.day + a_day;
		}
		if ( b < 0 ) {
			b = b + LY;
		}
				 if ( ( (dt.year + 1) % 4 == 0 ) && ( (dt.year + 1) % 100 != 0 ) ){
				   if (a <= 2 ) {
					   b = b - 1;
				   } else {
					  b = b; 
				   }
		} else if ( (dt.year + 1) % 400 == 0 ) {
			   if (a <= 2 ) {
				   b = b - 1;
			   } else {
				  b = b; 
			   }
		} else {
			  if (a <= 2 ) {
				   b = b + 1;
			   } else {
				  b = b; 
			   }
		}
	 char dai_2[10];
	_sprintf(dai_2, "%d", b);
	text_out(dai_2, 80, 55);
		text_out("Day", 139, 55);
// -------- Tính còn nhiêu ngày nữa đến ngày number_time đó  ------------

int d;
  
  set_fg_color(COLOR_GREEN);
		text_out("Ngày", 5, 80);
	char dai_4[12];
	_sprintf(dai_4, "%02d/%02d/%02d", a_day , a, a_year);
	text_out(dai_4, 100, 80);
	text_out("Còn", 5, 105);
	int sum;
	if ( dt.month == 1) {
			d = montha - dt.day + a_day;
		}
		if ( dt.month == 2) {
			d = montha - month1 - dt.day + a_day;
		}
		if ( dt.month == 3) {
			d = montha - month2  - dt.day + a_day;
		}
		if ( dt.month == 4) {
			d = montha - month3 - dt.day + a_day;
		}
		if ( dt.month == 5) {
			d = montha - month4 - dt.day + a_day;
		}
		if ( dt.month == 6) {
			d = montha - month5 - dt.day + a_day;
		}
		if ( dt.month == 7) {
			d = montha - month6 - dt.day + a_day;
		}
		if ( dt.month == 7) {
			d = montha - month7 - dt.day + a_day;
		}
		if ( dt.month == 9) {
			
			d = montha - month8 - dt.day + a_day;
		
		}
		if ( dt.month == 10) {
			d = montha - month9 - dt.day + a_day;
		}
		if ( dt.month == 11) {
			d = montha - month10 - dt.day + a_day;
		}
		if ( dt.month == 12) {
			d = montha - month11 - dt.day + a_day;
		}
	
	for (int i = a_year ; i < dt.year ; i++){
		 if ( ( i % 4 == 0 ) && ( i % 100 != 0 ) ){
			   //LY = -1; Bỏ
			   sum = sum + 366;
		  } else if ( i % 400 == 0 ) {
			   //LY = -1;
			   sum = sum + 366;
		  } else {
			  sum = sum + 365;
		  }
	}
	if ( a >= 2 ) {
		if ( ( a_year % 4 == 0 ) && ( a_year % 100 != 0 ) ){
			   //LY = -1; Bỏ
			   sum = sum - 1;
		  } else if ( a_year % 400 == 0 ) {
			   //LY = -1;
			   sum = sum - 1;
		  } else {
			  sum = sum;
		  }
	}
	d = sum - d;
	 char dai_6[10];
	_sprintf(dai_6, "%d", d);
	text_out(dai_6, 80, 105);
		text_out("Day", 139, 105);
// -------- Từ ngày hôm nay đến ngày ở trên là bao nhiêu ngày rồi ------------

   int e = 5; //Tháng
  int monthe;
  if ( e == 1 ) {
	  monthe = 0;
  } else if ( e == 2 ) {
	  monthe = month1;
  }else if ( e == 3 ) {
	  monthe = month2;
  }else if ( e == 4 ) {
	  monthe = month3;
  }else if ( e == 5 ) {
	  monthe = month4;
  }else if ( e == 6 ) {
	  monthe = month5;
  }else if ( e == 7 ) {
	  monthe = month6;
  }else if ( e == 8 ) {
	  monthe = month7;
  }else if ( e == 9 ) {
	  monthe = month8;
  }else if ( e == 10 ) {
	  monthe = month9;
  }else if ( e == 11 ) {
	  monthe = month10;
  }else if ( e == 12 ) {
	  monthe = month11;
  }
  
  
    int f; 
	set_fg_color(COLOR_PURPLE);
	text_out("SN Bin", 5, 130);
	text_out("27/5/1999", 100, 130);
	text_out("Còn", 5, 155);
	
		if ( dt.month == 1) {
			f = monthe - dt.day + 27;
		}
		if ( dt.month == 2) {
			f = monthe - month1 - dt.day + 27;
		}
		if ( dt.month == 3) {
			f = monthe - month2  - dt.day + 27;
		}
		if ( dt.month == 4) {
			f = monthe - month3 - dt.day + 27;
		}
		if ( dt.month == 5) {
			f = monthe - month4 - dt.day + 27;
		}
		if ( dt.month == 6) {
			f = monthe - month5 - dt.day + 27;
		}
		if ( dt.month == 7) {
			f = monthe - month6 - dt.day + 27;
		}
		if ( dt.month == 8) {
			f = monthe - month7 - dt.day + 27;
		}
		if ( dt.month == 9) {
			
			f = monthe - month8 - dt.day + 27;
		
		}
		if ( dt.month == 10) {
			f = monthe - month9 - dt.day + 27;
		}
		if ( dt.month == 11) {
			f = monthe - month10 - dt.day + 27;
		}
		if ( dt.month == 12) {
			f = monthe - month11 - dt.day + 27;
		}
		if ( f < 0 ) {
			f = f + LY;
		}
	 char dai_3[10];
	_sprintf(dai_3, "%d", f);
	text_out(dai_3, 80, 155);
		text_out("Day", 139, 155);
		
		/*if ((dt.day == 16 ) && (dt.month == 4 ) ){
		create_and_show_notification(31, "Hello", "Xin chào, đây là test", "Bin");	
		}*
		
		if ((dt.day == 27 - 5 ) && (dt.month == e ) ){
		create_and_show_notification(27, "Sinh nhật Bin", "Còn 5 ngày là đến sinh nhật Bin: 27/5", "Bin");	
		}
		if ((dt.day == 27 - 4 ) && (dt.month == e ) ){
		create_and_show_notification(27, "Sinh nhật Bin", "Còn 4 ngày là đến sinh nhật Bin: 27/5", "Bin");	
		}
		if ((dt.day == 27 - 3 ) && (dt.month == e ) ){
		create_and_show_notification(27, "Sinh nhật Bin", "Còn 3 ngày là đến sinh nhật Bin: 27/5", "Bin");	
		}
		if ((dt.day == 27 - 2 ) && (dt.month == e ) ){
		create_and_show_notification(27, "Sinh nhật Bin", "Còn 2 ngày là đến sinh nhật Bin: 27/5", "Bin");	
		}
		if ((dt.day == 27 - 1 ) && (dt.month == e ) ){
		create_and_show_notification(27, "Sinh nhật Bin", "Còn 1 ngày là đến sinh nhật Bin: 27/5", "Bin");	
		}
		if ((dt.day == 27 - 0 ) && (dt.month == e ) ){
		create_and_show_notification(27, "Sinh nhật Bin", "Chúc mừng sinh nhật Bin, hôm nay: 27/5", "Bin");	
		}
// -------- Sinh nhật Bin ------------

  set_update_period(1, 600000);  
repaint_screen_lines(0, 176);    
}*/

void draw_board(){
 set_bg_color(COLOR_BLACK);
  fill_screen_bg();
  load_font();
		// отрисовка значений
		set_fg_color(COLOR_WHITE);
		text_out_center("I’m Tony Nguyen \nv.gd/i1DGK5 \nm.me/tony99.inc", 88, 5);
		text_out_center("Hello World \nHave a nice day", 88, 110);
		char clock_time[8]; 			//	текст время		12:34
		struct datetime_ dt;
		get_current_date_time(&dt);
		_sprintf(clock_time, "%02d:%02d:%02d", dt.hour, dt.min, dt.sec);
		text_out_font(4, clock_time, 20 , 70, 6); // печатаем результат(время) большими цифрами
		set_update_period(1, 500);
		repaint_screen_lines(0, 176); // обновляем строки экрана
		

};

void draw_score_screen(){
  set_fg_color(COLOR_WHITE);
  set_bg_color(COLOR_BLACK);
  fill_screen_bg();
  set_fg_color(COLOR_WHITE);
text_out_center("Xin chao Tony Nguyen", 88, 6);

  set_bg_color(COLOR_GREEN);
  set_fg_color(COLOR_BLACK);
  draw_filled_rect_bg(4, 54, 172, 85);
  set_bg_color(COLOR_GREEN);
  text_out_center("Lịch", 88, 58);


  set_bg_color(COLOR_YELLOW);
  set_fg_color(COLOR_BLACK);
  draw_filled_rect_bg(4, 90, 86, 133);
  set_bg_color(COLOR_YELLOW);
  text_out_center("Convert", 40, 104);

  set_bg_color(COLOR_AQUA);
  set_fg_color(COLOR_BLACK);
  draw_filled_rect_bg(90, 90, 172, 133);
  set_bg_color(COLOR_AQUA);
  text_out_center("ClOCK", 132, 104);
  
    set_bg_color(COLOR_PURPLE);
  set_fg_color(COLOR_BLACK);
  draw_filled_rect_bg(4, 140, 172, 169);
  set_bg_color(COLOR_PURPLE);
  text_out_center("MEMORY", 88, 147);
  
  /*struct datetime_ dt;
		get_current_date_time(&dt);
  
  if ((dt.day == 16 ) && (dt.month == 4 ) ){
		create_and_show_notification(18, "Kỉ niệm ngày quen nhau", "Còn 5 ngày là đến ngày kỉ niệm: 7/6", "Bin");	
		}*/

  repaint_screen_lines(0, 176);
 
}

void calend_screen_job(){
	struct calend_** 	calend_p = get_ptr_temp_buf_2(); 		//	указатель на указатель на данные экрана 
	struct calend_ *	calend = *calend_p;			//	указатель на данные экрана
	
	if ( calend->screen == 2 ) {
	 numpad();
repaint_screen_lines(0, 176);
set_update_period(1, 10);
  }
  else if ( calend->screen == 1 ) {
	 draw_month();
repaint_screen_lines(0, 176);
set_update_period(1, 6000);
  }
  else if ( calend->screen == 3 ) {
	 convert_1();
repaint_screen_lines(0, 176);
set_update_period(1, 60000);
  }
  else if ( calend->screen == 4 ) {
	 convert_2();
repaint_screen_lines(0, 176);
set_update_period(1, 60000);
  }
  else if ( calend->screen == 5 ) {
	 convert_3();
repaint_screen_lines(0, 176);
set_update_period(1, 60000);
  }
  else if ( calend->screen == 6 ) {
	 convert_money();
repaint_screen_lines(0, 176);
set_update_period(1, 60000);
  }	
  else if ( calend->screen == 7 ) {
	 keypad();
repaint_screen_lines(0, 176);
set_update_period(1, 60000);
  } else if ( calend->screen == 10 ) {
	 warningpad();
repaint_screen_lines(0, 176);
set_update_period(1, 60000);
  } else if ( calend->screen == 11 ) {
	 kiniem();
repaint_screen_lines(0, 176);
set_update_period(1, 60000);
  } /*else if ( calend->screen == 12 ) {
	 kiniem1();
repaint_screen_lines(0, 176);
set_update_period(1, 60000);
  }else if ( calend->screen == 14 ) {
	kiniem2();
repaint_screen_lines(0, 176);
set_update_period(1, 60000);
  }*/
  else if (calend->screen == 9) {
  draw_board();
repaint_screen_lines(0, 176);
set_update_period(1, 500);
 }
  else if ( calend->screen == 8 ) {
	  draw_score_screen();
repaint_screen_lines(0, 176);
set_update_period(1, 60000);
  }
  else {

repaint_screen_lines(0, 176);
set_update_period(1, 60000);
  }
	// khi đạt đến bộ đếm thời gian cập nhật, hãy thoát
	//show_menu_animate(calend->ret_f, (unsigned int)show_calend_screen, ANIMATE_LEFT);
}

int dispatch_calend_screen (void *param){
	struct calend_** 	calend_p = get_ptr_temp_buf_2(); 		//	указатель на указатель на данные экрана 
	struct calend_ *	calend = *calend_p;			//	указатель на данные экрана
	
	struct calend_opt_ calend_opt;					//	опции календаря
	
	struct datetime_ datetime;
	// получим текущую дату
	
		
	get_current_date_time(&datetime);
	unsigned int day;
	
	//	char text_buffer[32];	
	 struct gesture_ *gest = param;
	 int result = 0;
	 
	switch (gest->gesture){
		case GESTURE_CLICK: {
			
				
			// вибрация при любом нажатии на экран
			vibrate (1, 40, 0);
			switch (calend->screen){
		case 1: {if (calend->screen == 1){ 
			
			if ( gest->touch_pos_y < CALEND_Y_BASE ){ // кликнули по верхней строке
				if (gest->touch_pos_x < 44){
					if ( calend->year > 1600 ) calend->year--;
				} else 
				if (gest->touch_pos_x > (176-44)){
					if ( calend->year < 3000 ) calend->year++;
				} else {
					calend->day 	= datetime.day;
					calend->month 	= datetime.month;
					calend->year 	= datetime.year;
				}	

				 if ( (calend->year == datetime.year) && (calend->month == datetime.month) ){
					day = datetime.day;
				 } else {	
					day = 0;
				 }
				
				month = calend->month;
				year = calend->year;
					draw_month();
					repaint_screen_lines(1, 176);			
				
			} else { // кликнули в календарь
			
				calend->color_scheme = ((calend->color_scheme+1)%COLOR_SCHEME_COUNT);
						
				// сначала обновим экран
				if ( (calend->year == datetime.year) && (calend->month == datetime.month) ){
					day = datetime.day;
				 } else {	
					day = 0;
				 }
				 month = calend->month;
				year = calend->year;
				calend->screen = 1;
					draw_month();
					repaint_screen_lines(1, 176);			
					
				// потом запись опций во flash память, т.к. это долгая операция
				// TODO: 1. если опций будет больше чем цветовая схема - переделать сохранение, чтобы сохранять перед выходом.
				calend_opt.color_scheme = calend->color_scheme;	

				// запишем настройки в флэш память
				ElfWriteSettings(calend->proc->index_listed, &calend_opt, OPT_OFFSET_CALEND_OPT, sizeof(struct calend_opt_));
			}
	}}
			// продлить таймер выхода при бездействии через INACTIVITY_PERIOD с
			//set_update_period(1, INACTIVITY_PERIOD);
		case 8: {if (calend->screen == 8){ 
                                       if ( ( gest->touch_pos_y >50) && ( gest->touch_pos_y < 85)  && ( gest->touch_pos_x >= 1) &&  ( gest->touch_pos_x <= 176) ){
                                        
										calend->screen = 1;
                                       set_bg_color(COLOR_BLACK);
                                        fill_screen_bg();
										draw_month(); //Lịch
                                
                                        repaint_screen_lines(0, 176);
                                      } else if ( ( gest->touch_pos_y >90) && ( gest->touch_pos_y < 133)  && ( gest->touch_pos_x >= 1) &&  ( gest->touch_pos_x <= 90) ){
                                        
										calend->screen = 7;
										set_bg_color(COLOR_BLACK);
                                        fill_screen_bg();
										keypad(); //Convert
										 repaint_screen_lines(0, 176);
										
                                        
										
                                                           }else if ( ( gest->touch_pos_y >90) && ( gest->touch_pos_y < 133)  && ( gest->touch_pos_x > 90) &&  ( gest->touch_pos_x <= 176) ){
                                        
										calend->screen = 9;
										set_bg_color(COLOR_BLACK);
                                        fill_screen_bg();
										set_update_period(1, 500);
										 draw_board(); //Clock
										  repaint_screen_lines(0, 176);
										 
                                       
									
                                           
                                      } else if ( ( gest->touch_pos_y >143) && ( gest->touch_pos_y <= 176)  && ( gest->touch_pos_x >= 1) &&  ( gest->touch_pos_x <= 176) ){
                                        
										calend->screen = 11;
										set_bg_color(COLOR_BLACK);
                                        fill_screen_bg();
										set_update_period(1, 600000);
										 kiniem(); // Kỉ niệm
										  repaint_screen_lines(0, 176);
                                      } else {
                                       
									calend->screen = 1;
										draw_month();
								
                                      }
                                      break;
			}}
		
		case 3: {if (calend->screen == 3){ 
															calend->screen = 4;		 
															 set_bg_color(COLOR_BLACK);
																fill_screen_bg();
																convert_2();
																repaint_screen_lines(0, 176);
															  }
																 break;
															  }
									  case 9: {if (calend->screen == 9){ 
									calend->screen = 8;		 
									 set_bg_color(COLOR_BLACK);
										fill_screen_bg();
										draw_score_screen();
                                        repaint_screen_lines(0, 176);
                                      }
									     break;
			                          }
									  case 4: {if (calend->screen == 4){ 
									calend->screen = 5;		 
									 set_bg_color(COLOR_BLACK);
										fill_screen_bg();
										convert_3();
                                        repaint_screen_lines(0, 176);
                                      }
									     break;
			                          }
									   case 5: {if (calend->screen == 5){ 
									calend->screen = 3;		 
									 set_bg_color(COLOR_BLACK);
										fill_screen_bg();
										convert_1();
                                        repaint_screen_lines(0, 176);
                                      }
									     break;
			                          }
									   
									   case 11: {if ( calend->screen == 11 ){ 
									   set_bg_color(COLOR_BLACK);
                                        fill_screen_bg();
										
                                      calend->screen = 8;
									 draw_score_screen();
									 repaint_screen_lines(0, 176);
                                      }
									 
                                      break;
			                          }
			 case 7: {if ( calend->screen == 7 ){ 
			 							/*set_bg_color(COLOR_BLACK);
										fill_screen_bg();
										
										calend->screen = 7;
										
										keypad();*/	
                                  if (number > 210000000 ) {
										if ( ( gest->touch_pos_y >= 26) && ( gest->touch_pos_y <= 75)  && ( gest->touch_pos_x >= 132) &&  ( gest->touch_pos_x <= 176) ){
                                        number = number / 10;
										calend->screen = 7;
										keypad();
										numpad();		
                                        repaint_screen_lines(0, 176);
                                        
                                      } else if ( ( gest->touch_pos_y >0) && ( gest->touch_pos_y <= 25)  && ( gest->touch_pos_x >= 0) &&  ( gest->touch_pos_x <= 176) ){
                                        
										calend->screen = 3;
                                       set_bg_color(COLOR_BLACK);
										fill_screen_bg();
									convert_1();
                                 //Qua trang ket qua                               
                                        repaint_screen_lines(0, 176);
                                      } else if ( ( gest->touch_pos_y >= 126) && ( gest->touch_pos_y <= 176)  && ( gest->touch_pos_x >= 132) &&  ( gest->touch_pos_x <= 176) ){
                                        calend->screen = 3;
                                       set_bg_color(COLOR_BLACK);
										fill_screen_bg();
									convert_1();
                                 //Qua trang ket qua 
								 
                                        repaint_screen_lines(0, 176);
                                      } else {
										  calend->screen = 10;
										warningpad(); 
										numpad();	
										repaint_screen_lines(0, 176);										
									  }
								  } if (number <= 210000000 ) {								  
                                      if ( ( gest->touch_pos_y >0) && ( gest->touch_pos_y <= 25)  && ( gest->touch_pos_x >= 0) &&  ( gest->touch_pos_x <= 176) ){
                                        
										calend->screen = 3;
                                       set_bg_color(COLOR_BLACK);
										fill_screen_bg();
									convert_1();
                                 //Qua trang ket qua                               
                                        repaint_screen_lines(0, 176);
                                      }  //Hang dau tien
									  else if ( ( gest->touch_pos_y >= 26) && ( gest->touch_pos_y <= 75)  && ( gest->touch_pos_x >= 0) &&  ( gest->touch_pos_x <= 43) ){
                                        number = number * 10 + 1;
										calend->screen = 7;
										keypad();
										numpad();		
                                        repaint_screen_lines(0, 176);
										} else if ( ( gest->touch_pos_y >= 26) && ( gest->touch_pos_y <= 75)  && ( gest->touch_pos_x >= 44) &&  ( gest->touch_pos_x <= 87) ){
                                        number = number * 10 + 2;
										calend->screen = 7;
										keypad();
										numpad();		
                                        repaint_screen_lines(0, 176);
                                       
                                      } else if ( ( gest->touch_pos_y >= 26) && ( gest->touch_pos_y <= 75)  && ( gest->touch_pos_x >= 88) &&  ( gest->touch_pos_x <= 131) ){
                                         number = number * 10 + 3;
										calend->screen = 7;
										keypad();
										numpad();		
                                        repaint_screen_lines(0, 176);
                                        
									}else if ( ( gest->touch_pos_y >= 26) && ( gest->touch_pos_y <= 75)  && ( gest->touch_pos_x >= 132) &&  ( gest->touch_pos_x <= 176) ){
                                        number = number / 10;
										calend->screen = 7;
										keypad();
										numpad();		
                                        repaint_screen_lines(0, 176);
                                        
                                      } //Hang thu 2
									  else if ( ( gest->touch_pos_y >= 76) && ( gest->touch_pos_y <= 125)  && ( gest->touch_pos_x >= 0) &&  ( gest->touch_pos_x <= 43) ){
                                        number = number * 10 + 4;
										calend->screen = 7;
										keypad();
										numpad();		
                                        repaint_screen_lines(0, 176);
                                       
										} else if ( ( gest->touch_pos_y >= 76) && ( gest->touch_pos_y <= 125)  && ( gest->touch_pos_x >= 44) &&  ( gest->touch_pos_x <= 87) ){
                                        number = number * 10 + 5;
										calend->screen = 7;
										keypad();
										numpad();		
                                        repaint_screen_lines(0, 176);
                                       
                                      } else if ( ( gest->touch_pos_y >= 76) && ( gest->touch_pos_y <= 125)  && ( gest->touch_pos_x >= 88) &&  ( gest->touch_pos_x <= 131) ){
                                         number = number * 10 + 6;
										calend->screen = 7;
										keypad();
										numpad();		
                                        repaint_screen_lines(0, 176);
                                       
									}else if ( ( gest->touch_pos_y >= 76) && ( gest->touch_pos_y <= 125)  && ( gest->touch_pos_x >= 132) &&  ( gest->touch_pos_x <= 176) ){
                                         number = number * 10 + 0;
										calend->screen = 7;
										keypad();
										numpad();		
                                        repaint_screen_lines(0, 176);
                                       
                                      } //Hang Thu 3
									  else if ( ( gest->touch_pos_y >= 126) && ( gest->touch_pos_y <= 176)  && ( gest->touch_pos_x >= 0) &&  ( gest->touch_pos_x <= 43) ){
                                        number = number * 10 + 7;
										calend->screen = 7;
										keypad();
										numpad();		
                                        repaint_screen_lines(0, 176);
                                      
										} else if ( ( gest->touch_pos_y >= 126) && ( gest->touch_pos_y <= 176)  && ( gest->touch_pos_x >= 44) &&  ( gest->touch_pos_x <= 87) ){
                                        number = number * 10 + 8;
										calend->screen = 7;
										keypad();
										numpad();		
                                        repaint_screen_lines(0, 176);
                                        
                                      } else if ( ( gest->touch_pos_y >= 126) && ( gest->touch_pos_y <= 176)  && ( gest->touch_pos_x >= 88) &&  ( gest->touch_pos_x <= 131) ){
                                         number = number * 10 + 9;
										calend->screen = 7;
										keypad();
										numpad();		
                                        repaint_screen_lines(0, 176);
                                      
									}else if ( ( gest->touch_pos_y >= 126) && ( gest->touch_pos_y <= 176)  && ( gest->touch_pos_x >= 132) &&  ( gest->touch_pos_x <= 176) ){
                                        calend->screen = 3;
                                       set_bg_color(COLOR_BLACK);
										fill_screen_bg();
									convert_1();
                                 //Qua trang ket qua 
								 
                                        repaint_screen_lines(0, 176);
                                      }
								  }									  
									  break;
						  }
						  }
						 
						  case 10: {if ( calend->screen == 10 ){ 
									   set_bg_color(COLOR_BLACK);
                                        fill_screen_bg();
										if ( ( gest->touch_pos_y >= 26) && ( gest->touch_pos_y <= 75)  && ( gest->touch_pos_x >= 132) &&  ( gest->touch_pos_x <= 176) ){
                                        number = number / 10;
										calend->screen = 7;
										keypad();
										numpad();		
                                        repaint_screen_lines(0, 176);
                                        
                                      } else if ( ( gest->touch_pos_y >0) && ( gest->touch_pos_y <= 25)  && ( gest->touch_pos_x >= 0) &&  ( gest->touch_pos_x <= 176) ){
                                        
										calend->screen = 3;
                                       set_bg_color(COLOR_BLACK);
										fill_screen_bg();
									convert_1();
                                 //Qua trang ket qua                               
                                        repaint_screen_lines(0, 176);
                                      } else if ( ( gest->touch_pos_y >= 126) && ( gest->touch_pos_y <= 176)  && ( gest->touch_pos_x >= 132) &&  ( gest->touch_pos_x <= 176) ){
                                        calend->screen = 3;
                                       set_bg_color(COLOR_BLACK);
										fill_screen_bg();
									convert_1();
                                 //Qua trang ket qua 
								 
                                        repaint_screen_lines(0, 176);
                                      } else {
										   number = number / 10;
										calend->screen = 7;
										keypad();
										numpad();		
                                        repaint_screen_lines(0, 176);
																				
									  }
                                      }
									 
                                      break;
			                          }
						  
							  
                                      break;
                    
					calend->screen = 1;
					draw_month();
					 repaint_screen_lines(0, 176);
                          }

                          break;
                        };
		
		case GESTURE_SWIPE_RIGHT: {	//	свайп направо
		/*set_bg_color(COLOR_BLACK);
                                        fill_screen_bg();
                                      calend->screen = 23;
									  numpad();
									 repaint_screen_lines(0, 176);*/
		vibrate (1, 40, 0);
		switch (calend->screen){
			
		
		case 1: {if ( calend->screen == 1 ){ 
									   set_bg_color(COLOR_BLACK);
                                        fill_screen_bg();
										
                                      calend->screen = 8;
									  
									  draw_score_screen();
									 repaint_screen_lines(0, 176);
                                      }
									 
                                      break;
			                          }
		case 2: {if ( calend->screen == 7 ){ 
									   set_bg_color(COLOR_BLACK);
                                        fill_screen_bg();
									calend->screen = 3;
									convert_1();
									 repaint_screen_lines(0, 176);
                                      }
									 
                                      break;
			                          }
									  case 3: {if ( calend->screen == 3 ){ 
									   set_bg_color(COLOR_BLACK);
                                        fill_screen_bg();
									calend->screen = 6;
									convert_money();
									 repaint_screen_lines(0, 176);
                                      }
									 
                                      break;
			                          }
									  case 4: {if ( calend->screen == 4 ){ 
									   set_bg_color(COLOR_BLACK);
                                        fill_screen_bg();
									calend->screen = 6;
									convert_money();
									 repaint_screen_lines(0, 176);
                                      }
									 
                                      break;
			                          }
									  case 5: {if ( calend->screen == 5 ){ 
									   set_bg_color(COLOR_BLACK);
                                        fill_screen_bg();
									calend->screen = 6;
									convert_money();
									 repaint_screen_lines(0, 176);
                                      }
									 
                                      break;
			                          }
									  case 6: {if ( calend->screen == 6 ){ 
									   set_bg_color(COLOR_BLACK);
                                        fill_screen_bg();
									calend->screen = 3;
									convert_1();
									 repaint_screen_lines(0, 176);
                                      }
									 
                                      break;
			                          }
							
							 /*calend->screen = 1;
				draw_month();
						repaint_screen_lines(0, 176);	*/
							
							
							  }                              
								break;
                              };	
		case GESTURE_SWIPE_LEFT: {	// справа налево
	
			if ( get_left_side_menu_active()){
					set_update_period(0,0);
					
					void* show_f = get_ptr_show_menu_func();

					// запускаем dispatch_left_side_menu с параметром param в результате произойдет запуск соответствующего бокового экрана
					// при этом произойдет выгрузка данных текущего приложения и его деактивация.
					dispatch_left_side_menu(param);
										
					if ( get_ptr_show_menu_func() == show_f ){
						// если dispatch_left_side_menu отработал безуспешно (листать некуда) то в show_menu_func по прежнему будет 
						// содержаться наша функция show_calend_screen, тогда просто игнорируем этот жест
						
						// продлить таймер выхода при бездействии через INACTIVITY_PERIOD с
						set_update_period(1, INACTIVITY_PERIOD);
						return 0;
					}

										
					//	если dispatch_left_side_menu отработал, то завершаем наше приложение, т.к. данные экрана уже выгрузились
					// на этом этапе уже выполняется новый экран (тот куда свайпнули)
					
					
					Elf_proc_* proc = get_proc_by_addr(main);
					proc->ret_f = NULL;
					
					elf_finish(main);	//	выгрузить Elf из памяти
					return 0;
				} else { 			//	если запуск не из быстрого меню, обрабатываем свайпы по отдельности
					switch (gest->gesture){
						case GESTURE_SWIPE_RIGHT: {	//	свайп направо
							return show_menu_animate(calend->ret_f, (unsigned int)show_calend_screen, ANIMATE_RIGHT);	
							break;
						}
						case GESTURE_SWIPE_LEFT: {	// справа налево
							//	действие при запуске из меню и дальнейший свайп влево
							
							
							break;
						}
					} /// switch (gest->gesture)
				}

			break;
		};	//	case GESTURE_SWIPE_LEFT:
		
		
		case GESTURE_SWIPE_UP: {	// свайп вверх
			switch (calend->screen){
			case 1: {if (calend->screen == 1){ 
				
				
			if ( calend->month < 12 ) 
					calend->month++;
			else {
					calend->month = 1;
					calend->year++;
			}
			
			if ( (calend->year == datetime.year) && (calend->month == datetime.month) )
				day = datetime.day;
			else	
				day = 0;
			month = calend->month;
				year = calend->year;
				calend->screen = 1;
			draw_month();
			repaint_screen_lines(1, 176);
			
			// продлить таймер выхода при бездействии через INACTIVITY_PERIOD с
			//set_update_period(1, INACTIVITY_PERIOD);
	}}
			
			case 7: {if (calend->screen == 7){ 
										 calend->screen = 1;
										 set_bg_color(COLOR_BLACK);
										fill_screen_bg();
										 draw_month();
									 repaint_screen_lines(0, 176);
                                      }
									     break;
			                          }
									  case 3: {if (calend->screen == 3){ 
									calend->screen = 7;		 
									 set_bg_color(COLOR_BLACK);
									
										fill_screen_bg();
										
										keypad();	
										numpad();
                                        repaint_screen_lines(0, 176);
                                      }
									     break;
			                          }
									  case 4: {if (calend->screen == 4){ 
									calend->screen = 7;		 
									 set_bg_color(COLOR_BLACK);
								
										fill_screen_bg();
										
										keypad();
										numpad();										
                                        repaint_screen_lines(0, 176);
                                      }
									     break;
			                          }
									  case 5: {if (calend->screen == 5){ 
									calend->screen = 7;		 
									 set_bg_color(COLOR_BLACK);
									
										fill_screen_bg();
										
										keypad();	
										numpad();
                                        repaint_screen_lines(0, 176);
                                      }
									     break;
			                          }
									  case 6: {if (calend->screen == 6){ 
									calend->screen = 7;		 
									 set_bg_color(COLOR_BLACK);
									
										fill_screen_bg();
										
										keypad();
										numpad();										
                                        repaint_screen_lines(0, 176);
                                      }
									     break;
			                          }
									  case 8: {if (calend->screen == 8){ 
										iwdg_reboot();
                                        repaint_screen_lines(0, 176);
                                      }
									     break;
			                          }
							 break;
                     /*calend->screen = 1;
					draw_month();
					repaint_screen_lines(1, 176);*/
                          }
                             break;
                           };
		case GESTURE_SWIPE_DOWN: {	// свайп вниз
			if ( calend->month > 1 ) 
					calend->month--;
			else {
					calend->month = 12;
					calend->year--;
			}
			
			if ( (calend->year == datetime.year) && (calend->month == datetime.month) )
				day = datetime.day;
			else	
				day = 0;
			month = calend->month;
				year = calend->year;
			draw_month();			
			repaint_screen_lines(1, 176);
			
			// продлить таймер выхода при бездействии через INACTIVITY_PERIOD с
			//set_update_period(1, INACTIVITY_PERIOD);
			break;
			
			
		};		
		default:{	// что-то пошло не так...
			break;
		};		
		
	}
	
	
	return result;
};
/*
 * (C) Copyright 2000
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/* #define	DEBUG	*/

#include <common.h>
#include <command.h>

#ifdef CFG_HUSH_PARSER
#include <hush.h>
#endif

#ifdef CONFIG_SILENT_CONSOLE
DECLARE_GLOBAL_DATA_PTR;
#endif

extern int reset_button_status(void);
extern void all_led_on(void);
extern void all_led_off(void);
extern void wan_led_on(void);
extern void wan_led_off(void);
extern void wlan_led_on(void);
extern void wlan_led_off(void);
extern int NetLoopHttpd(void);

#define MAX_DELAY_STOP_STR 32

static char *delete_char(char *buffer, char *p, int *colp, int *np, int plen);
static int parse_line(char *, char *[]);
//#if defined(CONFIG_BOOTDELAY) && (CONFIG_BOOTDELAY >= 0)
static int abortboot(int);
//#endif
#define	endtick(seconds) (get_ticks() + (uint64_t)(seconds) * get_tbclk())

char console_buffer[CFG_CBSIZE]; /* console I/O buffer	*/
static char erase_seq[] = "\b \b"; /* erase sequence	*/
static char tab_seq[] = "        "; /* used to expand TABs	*/

/***************************************************************************
 * Watch for 'delay' seconds for autoboot stop or autoboot delay string.
 * returns: 0 -  no key string, allow autoboot
 *          1 - got key string, abort
 */
#if defined(CONFIG_BOOTDELAY) && (CONFIG_BOOTDELAY >= 0)
#if defined(CONFIG_AUTOBOOT_KEYED)
static __inline__ int abortboot(int bootdelay)
{
	printf("  _____     ____    __  __   _____   _   _    ____   \n");
	printf(" |  __ \\   / __ \\  |  \\/  | |_   _| | \\ | |  / __ \\  \n");
	printf(" | |  | | | |  | | | \\  / |   | |   |  \\| | | |  | | \n");
	printf(" | |  | | | |  | | | |\\/| |   | |   | . ` | | |  | | \n");
	printf(" | |__| | | |__| | | |  | |  _| |_  | |\\  | | |__| | \n");
	printf(" |_____/   \\____/  |_|  |_| |_____| |_| \\_|  \\____/  \n");
	printf("\n");
	int abort = 0;
	uint64_t etime = endtick(bootdelay);
	struct
	{
		char* str;
		u_int len;
		int retry;
	}
	delaykey [] =
	{
		{ str: getenv ("bootdelaykey"),  retry: 1 },
		{ str: getenv ("bootdelaykey2"), retry: 1 },
		{ str: getenv ("bootstopkey"),   retry: 0 },
		{ str: getenv ("bootstopkey2"),  retry: 0 },
	};

	char presskey [MAX_DELAY_STOP_STR];
	u_int presskey_len = 0;
	u_int presskey_max = 0;
	u_int i;

#ifdef CONFIG_SILENT_CONSOLE
	if (gd->flags & GD_FLG_SILENT) {
		/* Restore serial console */
		console_assign (stdout, "serial");
		console_assign (stderr, "serial");
	}
#endif

#  ifdef CONFIG_AUTOBOOT_PROMPT
	printf (CONFIG_AUTOBOOT_PROMPT, bootdelay);
#  endif

#  ifdef CONFIG_AUTOBOOT_DELAY_STR
	if (delaykey[0].str == NULL)
		delaykey[0].str = CONFIG_AUTOBOOT_DELAY_STR;
#  endif
#  ifdef CONFIG_AUTOBOOT_DELAY_STR2
	if (delaykey[1].str == NULL)
		delaykey[1].str = CONFIG_AUTOBOOT_DELAY_STR2;
#  endif
#  ifdef CONFIG_AUTOBOOT_STOP_STR
	if (delaykey[2].str == NULL)
		delaykey[2].str = CONFIG_AUTOBOOT_STOP_STR;
#  endif
#  ifdef CONFIG_AUTOBOOT_STOP_STR2
	if (delaykey[3].str == NULL)
		delaykey[3].str = CONFIG_AUTOBOOT_STOP_STR2;
#  endif

	for (i = 0; i < sizeof(delaykey) / sizeof(delaykey[0]); i ++) {
		delaykey[i].len = delaykey[i].str == NULL ?
				    0 : strlen (delaykey[i].str);
		delaykey[i].len = delaykey[i].len > MAX_DELAY_STOP_STR ?
				    MAX_DELAY_STOP_STR : delaykey[i].len;

		presskey_max = presskey_max > delaykey[i].len ?
				    presskey_max : delaykey[i].len;

#  if DEBUG_BOOTKEYS
		printf("%s key:<%s>\n",
		       delaykey[i].retry ? "delay" : "stop",
		       delaykey[i].str ? delaykey[i].str : "NULL");
#  endif
	}

	/* In order to keep up with incoming data, check timeout only
	 * when catch up.
	 */
	while (!abort && get_ticks() <= etime) {
		for (i = 0; i < sizeof(delaykey) / sizeof(delaykey[0]); i ++) {
			if (delaykey[i].len > 0 &&
			    presskey_len >= delaykey[i].len &&
			    memcmp (presskey + presskey_len - delaykey[i].len,
				    delaykey[i].str,
				    delaykey[i].len) == 0) {
#  if DEBUG_BOOTKEYS
				printf("got %skey\n",
				       delaykey[i].retry ? "delay" : "stop");
#  endif

#  ifdef CONFIG_BOOT_RETRY_TIME
				/* don't retry auto boot */
				if (! delaykey[i].retry)
					retry_time = -1;
#  endif
				abort = 1;
			}
		}

		if (tstc()) {
			if (presskey_len < presskey_max) {
				presskey [presskey_len ++] = getc();
			}
			else {
				for (i = 0; i < presskey_max - 1; i ++)
					presskey [i] = presskey [i + 1];

				presskey [i] = getc();
			}
		}
	}
#  if DEBUG_BOOTKEYS
	if (!abort)
		puts ("key timeout\n");
#  endif

#ifdef CONFIG_SILENT_CONSOLE
	if (abort) {
		/* permanently enable normal console output */
		gd->flags &= ~(GD_FLG_SILENT);
	} else if (gd->flags & GD_FLG_SILENT) {
		/* Restore silent console */
		console_assign (stdout, "nulldev");
		console_assign (stderr, "nulldev");
	}
#endif

	return abort;
}

# else	/* !defined(CONFIG_AUTOBOOT_KEYED) */

static __inline__ int abortboot(int bootdelay){
	int abort = 0;

#ifdef CONFIG_SILENT_CONSOLE
	if(gd->flags & GD_FLG_SILENT){
		/* Restore serial console */
		console_assign(stdout, "serial");
		console_assign(stderr, "serial");
	}
#endif

	printf("  _____     ____    __  __   _____   _   _    ____   \n");
	printf(" |  __ \\   / __ \\  |  \\/  | |_   _| | \\ | |  / __ \\  \n");
	printf(" | |  | | | |  | | | \\  / |   | |   |  \\| | | |  | | \n");
	printf(" | |  | | | |  | | | |\\/| |   | |   | . ` | | |  | | \n");
	printf(" | |__| | | |__| | | |  | |  _| |_  | |\\  | | |__| | \n");
	printf(" |_____/   \\____/  |_|  |_| |_____| |_| \\_|  \\____/  \n");
	printf("\n");

	if(bootdelay > 0){
#ifdef CONFIG_MENUPROMPT
		printf(CONFIG_MENUPROMPT, bootdelay);
#else
		printf("Hit any key to stop autoboot: %d ", bootdelay);
#endif

		while((bootdelay > 0) && (!abort)){
			int i;

			--bootdelay;

			/* delay 100 * 10ms */
			for(i = 0; !abort && i < 100; ++i){

				/* we got a key press	*/
				if(tstc()){
					/* don't auto boot	*/
					abort = 1;
					/* no more delay	*/
					bootdelay = 0;
					/* consume input	*/
					(void) getc();
					break;
				}
				udelay(10000);
			}

			printf("\b\b%d ", bootdelay);
		}

		printf("\n\n");
	}

#ifdef CONFIG_SILENT_CONSOLE
	if(abort){
		/* permanently enable normal console output */
		gd->flags &= ~(GD_FLG_SILENT);
	} else if(gd->flags & GD_FLG_SILENT){
		/* Restore silent console */
		console_assign(stdout, "nulldev");
		console_assign(stderr, "nulldev");
	}
#endif

	return(abort);
}
#endif  /* defined(CONFIG_AUTOBOOT_KEYED)*/
#endif	/* CONFIG_BOOTDELAY >= 0  */

/*copy art from 4M to 16M*/
int copyart(int has_art_final){
	//volatile unsigned long *art_cal=(volatile unsigned long *)0x9f3f1000;
	volatile unsigned long *abeg=(volatile unsigned long *)0x9f3f1138;
	int has_art_cal=0;
	int rc=0;
	if((*abeg & 0x0000ffff)==0x4142 && *(abeg+1)==0x45473132){
		has_art_cal=1;
	}
	if(has_art_cal){
		printf(" _____________________________________________________________ \n");
		printf(" | 1. Device just calibrated, need to copy art.%14c|\n",' ');
		if(has_art_final){
			printf(" |%10cOverwriting...%35c|\n",' ',' ');
		}else{
			printf(" |%10cCopy it to final... %29c|\n",' ',' ');
		}
		rc=run_command("erase 0x9fff0000 +0x10000",0);
		if(rc==1) rc=run_command("cp.b 0x9f3f1000 0x80060000 0x0f000",0);
		if(rc==1) rc=run_command("cp.b 0x80060000 0x9fff1000 0xf000",0);
		if(rc==1) printf(" |%6cdone%49c|\n",' ',' '); else printf(" |%6cfailed%47c|\n",' ',' ');
		if(rc==1){
			rc=run_command("run lu",0);
			if(rc==1) run_command("reset",0);
		}
	}
	return rc;
}
/*
 * Check the calibration status
 * -1: no art found, stop
 * 0 : not calibrated, booting calibration firmware
 * 1 : calibrated, and but need to write config
 * 2 : calibrated, and config ready
 */
int calibration_status(void){

	int ret=-1;
	int calibrated=0;
	int has_art_final=0;
	int has_config=0;
	volatile unsigned short *art_final=(volatile unsigned short *)0x9fff1000;
	volatile unsigned int *config_data=(volatile unsigned int *)0x9fff0010;
	volatile unsigned char *v2=(volatile unsigned char *)0x9fff108f;
	volatile unsigned char *v3=(volatile unsigned char *)0x9fff1095;
	volatile unsigned char *v4=(volatile unsigned char *)0x9fff109b;


	if(*config_data!=0xffffffff){
		has_config=1;
	}
	//already calibrated
	if(*v2!=0 && *v3!=0 && *v4!=0){
		calibrated=1;
	}

	if(*art_final==0x0202){
		has_art_final=1;
	}
	//copyart(has_art_final);

	if(has_art_final){
		if(calibrated){
			if(has_config){
				printf("Device calibrated. Booting ...\n");
				ret=2;
			}else{
				printf("Device calibrated, need to write configure ...\n");;
				ret=1;
			}
		}else{
			printf("Device not calibrated. Booting the calibration firmware...\n");
			ret=0;
		}
	}else{
		printf("Cannot find art, please flash the default art first.\n");
		ret=-1;
	}


//	if(!has_art_cal){ //没有发现刚刚校准后的art
//		if(has_art_final){ //应该已经校准
//			printf("Calibration ready. Booting...\n");
//			ret=1;
//		}else{	//没有刚刚校准的art，也没有最后的art，应该是出错了！
//			printf("cannot find art, please calibrate!!!!!!!!!!\n");
//			ret=-1;
//		}
//	}else{ //发现刚刚校准的art
//		printf(" _____________________________________________________________ \n");
//		printf(" | 1. Device just calibrated, need to copy art.%14c|\n",' ');
//		if(has_art_final){
//			printf(" |%10cOverwriting...%35c|\n",' ',' ');
//		}else{
//			printf(" |%10cCopy it to final... %29c|\n",' ',' ');
//		}
//		rc=run_command("erase 0x9fff0000 +0x10000",0);
//		if(rc==1) rc=run_command("cp.b 0x9f3f1000 0x80060000 0x0f000",0);
//		if(rc==1) rc=run_command("cp.b 0x80060000 0x9fff1000 0xf000",0);
//		if(rc==1) printf(" |%6cdone%49c|\n",' ',' '); else printf(" |%6cfailed%47c|\n",' ',' ');
//		ret=0;
//	}
	return ret;
}

/*
 * Now only need to write config
 * */
int upgrade_firmware(void){
	int rc=0;
	//write mac address
	printf("  ___________________________________________________________ \n");
	printf(" | ** Writing MAC info%39c|\n",' ');
	rc=run_command("run lc",0);
//	rc=run_command("tftp 0x81000000 config.bin",0);
//	if(rc==1) rc=run_command("cp.b 0x9fff1000 0x80060000 0xf000 && erase 0x9fff0000 +0x10000 && cp.b 0x81000000 0x9fff0000 $filesize && cp.b 0x80060000 0x9fff1000 0xf000",0);
//	if(rc==1) rc=run_command("cp.b 0x9fff1000 0x80060000 0xf000",0);
//	if(rc==1) rc=run_command("erase 0x9fff0000 +0x10000",0);
//	if(rc==1) rc=run_command("cp.b 0x81000000 0x9fff0000 $filesize",0);
//	if(rc==1) rc=run_command("cp.b 0x80060000 0x9fff1000 0xf000",0);

//	if(rc==1) {
//		printf(" |%6cdone%49c|\n",' ',' ');
//	}else{
//		printf(" |%6cfailed%47c|\n",' ',' ');
//	}
	//write firmware
//	printf(" | 3. Writing firmware%39c|\n",' ');
//	all_led_on(); //flash red, turn off green, off/of is reversed
//	if(rc==1) rc=run_command("tftp 0x81000000 openwrt-domino.bin",0);
//	//red_led_off(); //flash green
//	if(rc==1) rc=run_command("erase 0x9f050000 +0x7f0000",0);
//	all_led_on();
//	//red_led_on(); //red on only
//	if(rc==1) rc=run_command("cp.b 0x81000000 0x9f050000 0x7f0000",0);
	if(rc==1){
		run_command("run lf",0);
	}
	if(rc==1) {
		printf(" |%6cDone, resetting...%35c|\n",' ',' ');
		printf(" |___________________________________________________________|\n");
		run_command("reset",0);
	}else{
		printf(" |%6cFailed.%46c|\n",' ',' ');
		printf(" |___________________________________________________________|\n");
	}
	return rc;
}


/****************************************************************************/

void main_loop(void){
#ifndef CFG_HUSH_PARSER
	static char lastcommand[CFG_CBSIZE] = { 0, };
	int len;
	int rc = 1;
	int flag;
#endif
	int counter = 0;

#if defined(CONFIG_BOOTDELAY) && (CONFIG_BOOTDELAY >= 0)
	char *s;
	int bootdelay;
#endif /* defined(CONFIG_BOOTDELAY) && (CONFIG_BOOTDELAY >= 0) */

#ifdef CFG_HUSH_PARSER
	u_boot_hush_start();
#endif

#if defined(CONFIG_BOOTDELAY) && (CONFIG_BOOTDELAY >= 0)
	// get boot delay (seconds)
	s = getenv("bootdelay");
	bootdelay = s ? (int)simple_strtol(s, NULL, 10) : CONFIG_BOOTDELAY;

	// get boot command
	s = getenv("bootcmd");

#if !defined(CONFIG_BOOTCOMMAND)
#error "CONFIG_BOOTCOMMAND not defined!"
#endif

	if(!s){
		setenv("bootcmd", CONFIG_BOOTCOMMAND);
	}

	s = getenv("bootcmd");

	// are we going to run web failsafe mode, U-Boot console, U-Boot netconsole or just boot command?
	if(reset_button_status()){

#ifdef CONFIG_SILENT_CONSOLE
		if(gd->flags & GD_FLG_SILENT){
			/* Restore serial console */
			console_assign(stdout, "serial");
			console_assign(stderr, "serial");
		}

		/* enable normal console output */
		gd->flags &= ~(GD_FLG_SILENT);
#endif

		// wait 0,5s
		milisecdelay(500);

		printf("Press reset button for at least:\n- %d sec. to run web failsafe mode\n- %d sec. to run U-Boot console\n- %d sec. to run U-Boot netconsole\n\n",
				CONFIG_DELAY_TO_AUTORUN_HTTPD,
				CONFIG_DELAY_TO_AUTORUN_CONSOLE,
				CONFIG_DELAY_TO_AUTORUN_NETCONSOLE);

		printf("Reset button is pressed for: %2d ", counter);

		while(reset_button_status()){

			// LED ON and wait 0,15s
			wlan_led_on();
			milisecdelay(150);

			// LED OFF and wait 0,85s
			wlan_led_off();
			milisecdelay(850);

			counter++;

			// how long the button is pressed?
			printf("\b\b\b%2d ", counter);

			//turn on Red LED to show httpd started
			if(counter==CONFIG_DELAY_TO_AUTORUN_HTTPD){
				wan_led_on();
			}
			//turn off Red LED to show uboot console
			else if(counter==CONFIG_DELAY_TO_AUTORUN_CONSOLE){
				wan_led_off();
			}
			//turn on Red LED again to show netconsole
			else if(counter==CONFIG_DELAY_TO_AUTORUN_NETCONSOLE){
				wan_led_on();
			}
			if(!reset_button_status()){
				break;
			}

			if(counter >= CONFIG_MAX_BUTTON_PRESSING){
				break;
			}
		}

		//all_led_off();

		if(counter > 0){

			// run web failsafe mode
			if(counter >= CONFIG_DELAY_TO_AUTORUN_HTTPD && counter < CONFIG_DELAY_TO_AUTORUN_CONSOLE){
				printf("\n\nButton was pressed for %d sec...\nHTTP server is starting for firmware update...\n\n", counter);
				NetLoopHttpd();
				bootdelay = -1;
			} else if(counter >= CONFIG_DELAY_TO_AUTORUN_CONSOLE && counter < CONFIG_DELAY_TO_AUTORUN_NETCONSOLE){
				printf("\n\nButton was pressed for %d sec...\nStarting U-Boot console...\n\n", counter);
				bootdelay = -1;
			} else if(counter >= CONFIG_DELAY_TO_AUTORUN_NETCONSOLE){
				printf("\n\nButton was pressed for %d sec...\nStarting U-Boot netconsole...\n\n", counter);
				bootdelay = -1;
				run_command("startnc", 0);
			} else {
				printf("\n\n## Error: button wasn't pressed long enough!\nContinuing normal boot...\n\n");
			}

		} else {
			printf("\n\n## Error: button wasn't pressed long enough!\nContinuing normal boot...\n\n");
		}

	}

	if(bootdelay >= 0 && s && !abortboot(bootdelay)){

		int status=calibration_status();
		/*是不是需要校准
		 * -1: no art found, stop
		 * 0 : not calibrated, booting calibration firmware
		 * 1 : calibrated, and but need to write config
		 * 2 : calibrated, and config ready
		*/
		if (status==0){ //start calibration firmware
			setenv("bootargs", "console=ttyS0,115200 root=31:03 rootfstype=squashfs init=/sbin/init mtdparts=ar7240-nor0:256k(u-boot),64k(u-boot-env),12288k(firmware),2752k(rootfs),896k(uImage),64k(NVRAM),64k(ART)");
			run_command("bootm 0x9ff00000",0);
		}else if(status==1){
			milisecdelay(1000);
			if(upgrade_firmware()!=1){ //write config, return 1 if succeed
				printf("Cannot write config, not booting.\n");
				goto mainloop;
			}
		}else if(status==-1){
			goto mainloop;
		}

		// try to boot
#ifndef CFG_HUSH_PARSER
			run_command(s, 0);
#else
			parse_string_outer(s, FLAG_PARSE_SEMICOLON | FLAG_EXIT_FROM_LOOP);
#endif

		//try to boot arduino yun if 1st try failed
		//setenv("bootargs", "console=ttyS0,115200 root=31:03 rootfstype=squashfs init=/sbin/init mtdparts=ar7240-nor0:256k(u-boot),64k(u-boot-env),12288k(firmware),2752k(rootfs),896k(uImage),64k(NVRAM),64k(ART)");
		//run_command("bootm 0x9fea0000",0);
		// something goes wrong!
		printf("\n## Error: failed to execute 'bootcmd'!\nHTTP server is starting for firmware update...\n\n");
		NetLoopHttpd();
	}
#endif	/* CONFIG_BOOTDELAY */

	/*
	 * Main Loop for Monitor Command Processing
	 */
mainloop:
#ifdef CFG_HUSH_PARSER
	parse_file_outer();
	/* This point is never reached */
	for (;;);
#else
	for(;;){
		len = readline(CFG_PROMPT);

		flag = 0; /* assume no special flags for now */
		if(len > 0){
			strcpy(lastcommand, console_buffer);
		} else if(len == 0){
			flag |= CMD_FLAG_REPEAT;
		}

		if(len == -1){
			puts("<INTERRUPT>\n");
		} else {
			rc = run_command(lastcommand, flag);
		}

		if(rc <= 0){
			/* invalid command or not repeatable, forget it */
			lastcommand[0] = 0;
		}
	}
#endif /* CFG_HUSH_PARSER */
}

/****************************************************************************/

/*
 * Prompt for input and read a line.
 * If  CONFIG_BOOT_RETRY_TIME is defined and retry_time >= 0,
 * time out when time goes past endtime (timebase time in ticks).
 * Return:	number of read characters
 *		-1 if break
 *		-2 if timed out
 */
int readline(const char * const prompt){
	char *p = console_buffer;
	int n = 0; /* buffer index		*/
	int plen = 0; /* prompt length	*/
	int col; /* output column cnt	*/
	char c;

	/* print prompt */
	if(prompt){
		plen = strlen(prompt);
		puts(prompt);
	}
	col = plen;

	for(;;){
		c = getc();

		/*
		 * Special character handling
		 */
		switch(c){
			case '\r': /* Enter		*/
			case '\n':
				*p = '\0';
				puts("\r\n");
				return(p - console_buffer);

			case '\0': /* nul			*/
				continue;

			case 0x03: /* ^C - break		*/
				console_buffer[0] = '\0'; /* discard input */
				return(-1);

			case 0x15: /* ^U - erase line	*/
				while(col > plen){
					puts(erase_seq);
					--col;
				}
				p = console_buffer;
				n = 0;
				continue;

			case 0x17: /* ^W - erase word 	*/
				p = delete_char(console_buffer, p, &col, &n, plen);
				while((n > 0) && (*p != ' ')){
					p = delete_char(console_buffer, p, &col, &n, plen);
				}
				continue;

			case 0x08: /* ^H  - backspace	*/
			case 0x7F: /* DEL - backspace	*/
				p = delete_char(console_buffer, p, &col, &n, plen);
				continue;

			default:
				/*
				 * Must be a normal character then
				 */
				if(n < CFG_CBSIZE - 2){
					if(c == '\t'){ /* expand TABs		*/
						puts(tab_seq + (col & 07));
						col += 8 - (col & 07);
					} else {
						++col; /* echo input		*/
						putc(c);
					}
					*p++ = c;
					++n;
				} else { /* Buffer full		*/
					putc('\a');
				}
		}
	}
}

/****************************************************************************/

static char * delete_char(char *buffer, char *p, int *colp, int *np, int plen){
	char *s;

	if(*np == 0){
		return(p);
	}

	if(*(--p) == '\t'){ /* will retype the whole line	*/
		while(*colp > plen){
			puts(erase_seq);
			(*colp)--;
		}
		for(s = buffer; s < p; ++s){
			if(*s == '\t'){
				puts(tab_seq + ((*colp) & 07));
				*colp += 8 - ((*colp) & 07);
			} else {
				++(*colp);
				putc(*s);
			}
		}
	} else {
		puts(erase_seq);
		(*colp)--;
	}
	(*np)--;
	return(p);
}

/****************************************************************************/

int parse_line(char *line, char *argv[]){
	int nargs = 0;

	while(nargs < CFG_MAXARGS){

		/* skip any white space */
		while((*line == ' ') || (*line == '\t')){
			++line;
		}

		if(*line == '\0'){ /* end of line, no more args	*/
			argv[nargs] = NULL;
			return(nargs);
		}

		argv[nargs++] = line; /* begin of argument string	*/

		/* find end of string */
		while(*line && (*line != ' ') && (*line != '\t')){
			++line;
		}

		if(*line == '\0'){ /* end of line, no more args	*/
			argv[nargs] = NULL;
			return(nargs);
		}

		*line++ = '\0'; /* terminate current arg	 */
	}

	printf("## Error: too many args (max. %d)\n", CFG_MAXARGS);

	return(nargs);
}

/****************************************************************************/

static void process_macros(const char *input, char *output){
	char c, prev;
	const char *varname_start = NULL;
	int inputcnt = strlen(input);
	int outputcnt = CFG_CBSIZE;
	int state = 0; /* 0 = waiting for '$'	*/
	/* 1 = waiting for '(' or '{' */
	/* 2 = waiting for ')' or '}' */
	/* 3 = waiting for '''  */

	prev = '\0'; /* previous character	*/

	while(inputcnt && outputcnt){
		c = *input++;
		inputcnt--;

		if(state != 3){
			/* remove one level of escape characters */
			if((c == '\\') && (prev != '\\')){
				if(inputcnt-- == 0){
					break;
				}

				prev = c;
				c = *input++;
			}
		}

		switch(state){
			case 0: /* Waiting for (unescaped) $	*/
				if((c == '\'') && (prev != '\\')){
					state = 3;
					break;
				}
				if((c == '$') && (prev != '\\')){
					state++;
				} else {
					*(output++) = c;
					outputcnt--;
				}
				break;
			case 1: /* Waiting for (	*/
				if(c == '(' || c == '{'){
					state++;
					varname_start = input;
				} else {
					state = 0;
					*(output++) = '$';
					outputcnt--;

					if(outputcnt){
						*(output++) = c;
						outputcnt--;
					}
				}
				break;
			case 2: /* Waiting for )	*/
				if(c == ')' || c == '}'){
					int i;
					char envname[CFG_CBSIZE], *envval;
					int envcnt = input - varname_start - 1; /* Varname # of chars */

					/* Get the varname */
					for(i = 0; i < envcnt; i++){
						envname[i] = varname_start[i];
					}
					envname[i] = 0;

					/* Get its value */
					envval = getenv(envname);

					/* Copy into the line if it exists */
					if(envval != NULL){
						while((*envval) && outputcnt){
							*(output++) = *(envval++);
							outputcnt--;
						}
					}
					/* Look for another '$' */
					state = 0;
				}
				break;
			case 3: /* Waiting for '	*/
				if((c == '\'') && (prev != '\\')){
					state = 0;
				} else {
					*(output++) = c;
					outputcnt--;
				}
				break;
		}
		prev = c;
	}

	if(outputcnt){
		*output = 0;
	}
}

/****************************************************************************
 * returns:
 *	1  - command executed, repeatable
 *	0  - command executed but not repeatable, interrupted commands are
 *	     always considered not repeatable
 *	-1 - not executed (unrecognized, bootd recursion or too many args)
 *           (If cmd is NULL or "" or longer than CFG_CBSIZE-1 it is
 *           considered unrecognized)
 *
 * WARNING:
 *
 * We must create a temporary copy of the command since the command we get
 * may be the result from getenv(), which returns a pointer directly to
 * the environment data, which may change magicly when the command we run
 * creates or modifies environment variables (like "bootp" does).
 */

int run_command(const char *cmd, int flag){
	cmd_tbl_t *cmdtp;
	char cmdbuf[CFG_CBSIZE]; /* working copy of cmd		*/
	char *token; /* start of token in cmdbuf	*/
	char *sep; /* end of token (separator) in cmdbuf */
	char finaltoken[CFG_CBSIZE];
	char *str = cmdbuf;
	char *argv[CFG_MAXARGS + 1]; /* NULL terminated	*/
	int argc, inquotes;
	int repeatable = 1;
	int rc = 0;

	clear_ctrlc(); /* forget any previous Control C */

	if(!cmd || !*cmd){
		return(-1); /* empty command */
	}

	if(strlen(cmd) >= CFG_CBSIZE){
		puts("## Error: command too long!\n");
		return(-1);
	}

	strcpy(cmdbuf, cmd);

	/* Process separators and check for invalid
	 * repeatable commands
	 */
	while(*str){

		/*
		 * Find separator, or string end
		 * Allow simple escape of ';' by writing "\;"
		 */
		for(inquotes = 0, sep = str; *sep; sep++){
			if((*sep == '\'') && (*(sep - 1) != '\\')){
				inquotes = !inquotes;
			}

			if(!inquotes && (*sep == ';') && (sep != str) && (*(sep - 1) != '\\')){
				break;
			}
		}

		/*
		 * Limit the token to data between separators
		 */
		token = str;
		if(*sep){
			str = sep + 1; /* start of command for next pass */
			*sep = '\0';
		} else {
			str = sep; /* no more commands for next pass */
		}

		/* find macros in this token and replace them */
		process_macros(token, finaltoken);

		/* Extract arguments */
		if((argc = parse_line(finaltoken, argv)) == 0){
			rc = -1; /* no command at all */
			continue;
		}

		/* Look up command in command table */
		if((cmdtp = find_cmd(argv[0])) == NULL){
			printf("## Error: unknown command '%s' - try 'help'\n\n", argv[0]);
			rc = -1; /* give up after bad command */
			continue;
		}

		/* found - check max args */
		if(argc > cmdtp->maxargs){
#ifdef CFG_LONGHELP
			if(cmdtp->help != NULL){
				printf("Usage:\n%s %s\n", cmdtp->name, cmdtp->help);
			} else {
				printf("Usage:\n%s %s\n", cmdtp->name, cmdtp->usage);
			}
#else
			printf("Usage:\n%s %s\n", cmdtp->name, cmdtp->usage);
#endif
			rc = -1;
			continue;
		}

		/* OK - call function to do the command */
		if((cmdtp->cmd)(cmdtp, flag, argc, argv) != 0){
			rc = -1;
		}

		repeatable &= cmdtp->repeatable;

		/* Did the user stop this? */
		if(had_ctrlc()){ /* if stopped then not repeatable */
			return(0);
		}
	}

	return(rc ? rc : repeatable);
}

/****************************************************************************/

#if (CONFIG_COMMANDS & CFG_CMD_RUN)
int do_run(cmd_tbl_t * cmdtp, int flag, int argc, char *argv[]){
	int i;

	if(argc < 2){
#ifdef CFG_LONGHELP
		if(cmdtp->help != NULL){
			printf("Usage:\n%s %s\n", cmdtp->name, cmdtp->help);
		} else {
			printf("Usage:\n%s %s\n", cmdtp->name, cmdtp->usage);
		}
#else
		printf("Usage:\n%s %s\n", cmdtp->name, cmdtp->usage);
#endif
		return(1);
	}

	for(i=1; i<argc; ++i){
		char *arg;

		if((arg = getenv(argv[i])) == NULL){
			printf("## Error: \"%s\" not defined\n", argv[i]);
			return(1);
		}
#ifndef CFG_HUSH_PARSER
		if(run_command(arg, flag) == -1){
			return(1);
		}
#else
		if (parse_string_outer(arg, FLAG_PARSE_SEMICOLON | FLAG_EXIT_FROM_LOOP) != 0){
			return(1);
		}
#endif /* CFG_HUSH_PARSER */
	}

	return(0);
}
#endif	/* CFG_CMD_RUN */

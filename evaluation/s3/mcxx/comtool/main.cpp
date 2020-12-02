

/* windows */
#ifdef WIN32
#pragma warning(disable:4996)
#define _CRT_SECURE_NO_DEPRECATE 1
#define _POSIX_
#include <windows.h>
#include <direct.h>
#include <crtdbg.h>
#include <float.h>
#else
/* linux and mac */
#include <unistd.h>
#endif

/* POSIX */
#include <stdio.h>

/* libs3 */
#include <core/libmpxx/mpxx_string.h>
#include <core/libmpxx/mpxx_cl2av.h>

#include <core/libmcxx/mcxx_dev232.h>
#include <core/libmcxx/mcxx_dev232_find.h>

static int cmd_loop(void);
static int main_command(char *cmd, char *res_buf, int cmd_size, int res_size);

static int quit_cmd(int ac, char **av);
static int help_cmd(int ac, char **av);
static int devlist_cmd(int ac, char **av);
static int open_cmd(int ac, char **av);
static int close_cmd(int ac, char **av);
static int devstat_cmd(int ac, char **av);
static int search_cmd(int ac, char **av);
static int command_cmd(int ac, char **av);

static int current_fd = 0;
static int errexit = 0;
static int prompt_mode = 1;
static int echo_mode = 0;
static char *wdir = NULL;

static struct com_hdl {
    s3::mcxx_dev232_handle_t dev_hdl;

    union {
	unsigned int flags;
	struct {
	    unsigned int dev_hdl:1;
	} f;
    } init;
} com_hdl = { 0 };

/* *INDENT-OFF* */
typedef struct command_table {
    int sub;
    char *cmd;
    int (*func)( int ac, char **av);
    char *help;
} command_table_t;

command_table_t ctbl[] = {
    { 0, "open", open_cmd,	"<com_path> : open  com port"},
    { 0, "close",close_cmd,	"close comport"},
    { 0, "devlist", devlist_cmd, "dump com port"},
    { 0, "devstat", devstat_cmd, "dump com device status"},
    { 0, "cmd"    , command_cmd, "send command and reply"},
    { 0, "search" , search_cmd, "search device"},
    { 0, "help",help_cmd,		""},
    { 0, "?", help_cmd,		" [all] - disp this message"},
    { 0, "q", quit_cmd,		" [-f] : quit"},
    { 0, NULL, NULL,		NULL}
};
/* *INDENT-ON* */

static void usage(const char *cmd)
{
	
    fprintf( stdout, "%s [options...] \n", cmd);
    fprintf( stdout, "\n");
    fprintf( stdout, "sorry : not impliment\n");

    return;
}

int main(int argc, char **argv)
{
    int result;

    if( argc >= 2 ) {
	int i;
	int opt_cnt;
	char *w;

	for( i=1; i<= (argc - 1); i++ ) {
            if (*(char *) argv[i] != '-')
                continue;

	    w = (char*)argv[i] + 1;

	    if( !mpxx_strcasecmp( w, "h" ) || !mpxx_strcasecmp( w, "help" ) ) {
		usage(argv[0]);
		exit(0);
	    }

	    if( !mpxx_strcasecmp( w, "errexit" ) ) {
		errexit = 1;
		continue;
	    }

	    if( !mpxx_strcasecmp( w, "wdir" ) ) {
		opt_cnt = 1;
		if (!((i + opt_cnt) < argc)) { /* option check */
		    fprintf( stdout, "Syntax Error\n");		
		    usage(argv[0]);
		    exit(1);
		}

		wdir = argv[i+1];
		continue;
	    }

	    if( !mpxx_strcasecmp( w, "noprompt" ) ) {
		prompt_mode = 0;
		continue;
	    }

	    if( !mpxx_strcasecmp( w, "echo" ) ) {
		echo_mode = 1;
		continue;
	    }

            fprintf( stdout," Invalid option\n");
            i = 9999;
            return -1;
            break;
	}
    }

    if(prompt_mode) {
	fprintf( stdout, "-------- EIA232 Serialport Command Tool ------------\n");
	fprintf( stdout, "   Copyright 2017 Sensor Technorogy Inc.,\n");
	fprintf( stdout, "            http://http://www.sensortechnology.co.jp\n");
	fprintf( stdout, "----------------------------------------------------\n");
	fprintf( stdout, "\n");
    }

#if 0 
    fprintf( stderr, "WARNING! : use Memory check routine for Windows\n");
    _CrtSetDbgFlag(
              _CRTDBG_ALLOC_MEM_DF
            | _CRTDBG_LEAK_CHECK_DF
          | _CRTDBG_CHECK_ALWAYS_DF
	 );

    // _CrtSetBreakAlloc(53);
#endif

    if( NULL != wdir ) {

#ifdef WIN32
	 result = _chdir(wdir);
#else
	 result = chdir(wdir);
#endif

	if (result) {
	    return 1;
	}
    }

    return cmd_loop();
}

static int cmd_loop(void)
{
    int status = -1;
    int result = -1;

    char cmd[512];
    char req_dat[512];
    int line=0;

    while (1) {
	memset(req_dat, 0, sizeof(req_dat));
	line++;

	if( prompt_mode ) {
	    fprintf(stdout, "%d> ", line);
	}

	fgets(req_dat, 512, stdin);

	if (sscanf(req_dat, "%s", cmd) < 1) {
	    continue;
	}

	if( echo_mode ) {
	    fprintf( stdout, "%d> %s", line, req_dat);
	    fflush( stdout );
	}

	if( cmd[0] == '!' ) {
	    system(req_dat + 1);
	    continue;
	}

	if( cmd[0] == '#' ) {
	    /* comment */
	    continue;
	}

	result =
	    main_command(req_dat, NULL, (int)strlen(req_dat), -1);
	if (result) {
	    status = result;
	    goto out;
	}

    }

    status = 0;
out:

    return status;
}

static int main_command(char *cmd, char *res_buf, int com_size, int res_size)
{

#define MAX_ARGC 64

    int status = -1;
    int result;
    int errors = 0;
    int i;

    char cmd_buf[256];
    char main_func[256];

    mpxx_cl2av_t marg;
    mpxx_cl2av_attr_t marg_attr = { 0 };

    mpxx_cl2av_init( &marg, " \t\n", '"',  &marg_attr);

    if (com_size > 256) {
	status = -1;
	goto out;
    }

    /* inital */
    memset(cmd_buf, 0, 256);

    strcpy(cmd_buf, cmd);
    result = mpxx_cl2av_execute( &marg, cmd);
    if (result) {
	fprintf( stderr, "Syntax Error\n");
	status = 0;
	goto out;
    }

    mpxx_strtolower(marg.argv[0], main_func, 256);

    for (i = 0; ctbl[i].cmd != NULL; i++) {
	if (!strcmp(ctbl[i].cmd, main_func)) {
	    errors = (ctbl[i].func) (marg.argc, marg.argv);
	    if (errors) {
		if (!ctbl[i].sub) {
		    fprintf( stdout, " %s : %s\n", marg.argv[0], strerror(errors));
		}
		status = 0;
		goto out;
	    }

	    break;
	}

    }

    if (ctbl[i].cmd == NULL) {
	fprintf( stdout, " %s : %s\n", marg.argv[0], "Unknwon command\n");
	status = 0;
	goto out;
    }

    status = 0;

  out:

    mpxx_cl2av_destroy( &marg);

    if( errexit && errors ) {
	fprintf( stdout,"error=%d\n", errors);
	status = errors;
    }

    return status;
}

static int quit_cmd(int ac, char **av)
{
    if (ac != 1)
	return 1;

//    fprintf( stderr, "hit any key\n");
//    fgetc(stdin);

    exit(0);
}

static void helpall(command_table_t * c);

int help_cmd(int ac, char **av)
{
    int status = -1;
    int i = 0;
    int m = 0;
    int all_mode = 0;

    char cmd[64];

    if (ac == 1) {
	for (i = 0; ctbl[i].cmd != NULL; i++) {
	    fprintf( stdout, "%-12s", ctbl[i].cmd);
	    m++;

	    if (m >= 4) {
		m = 0;
		fprintf( stdout, "\n");
	    }
	}
	status = 0;
	fprintf( stdout, "\n");
	goto out;
    }

    if (ac != 2) {
	status = -1;
    } else if (sscanf(av[1], "%s", cmd) != 1) {
	status = -1;
    } else
	status = 0;

    if (status) {
	fprintf( stdout, " %s [command]\n", av[0]);
	goto out;
    }

    if (!_stricmp(av[1], "all"))
	all_mode = 1;

    for (i = 0; ctbl[i].cmd != NULL; i++) {

	if (all_mode) {
	    fprintf( stdout, "%8s : %s\n", ctbl[i].cmd, ctbl[i].help);
	} else {
	    if (!strcmp(ctbl[i].cmd, cmd)) {
		fprintf( stdout, "%8s : %s\n", ctbl[i].cmd, ctbl[i].help);
		break;
	    }
	}
    }

    if (ctbl[i].cmd == NULL && !all_mode) {
	printf("%s : %s is unkown command\n", av[0], cmd);
    }

  out:

    return status;
}

static void helpall(command_table_t * c)
{
    int i;

    for (i = 0; c[i].cmd != NULL; i++) {
	fprintf( stdout, "%-10s : %s\n", c[i].cmd, c[i].help);
    }

    return;
}

static int devlist_cmd(int ac, char **av) {

    s3::mcxx_dev232_list_t *devcoms = NULL;
    size_t n;

    devcoms = s3::mcxx_dev232_new_devlist();
    if( devcoms == NULL ) {
	return ENODEV;
    }

    if( devcoms->num_tabs == 0 ) {
	return ENODEV;
    }

    for( n=0; n< devcoms->num_tabs; ++n) {
	fprintf( stdout, "[%u] : %s\n", n, devcoms->tabs[n].path);
    }

    s3::mcxx_dev232_delete_devlist( devcoms );

    return 0;
}

static int open_cmd(int ac, char **av) {
    int result;
    size_t retlen;
    s3::mcxx_dev232_attr_t dev_attr;
    const char *const logoff_str = "logdump 0";
    const char *const echooff_str = "echoback 0";
    char *const resbuf[256] = { 0 };

    if(com_hdl.init.f.dev_hdl) {
	return EALREADY;
    }

    if( ac != 2 ) {
	return EINVAL;
    }

    result = s3::mcxx_dev232_open( &com_hdl.dev_hdl, av[1]);
    if(result) {
	return result;
    }
    com_hdl.init.f.dev_hdl = 1;

    /* COMポート設定変更　*/
    result = s3::mcxx_dev232_get_attr( &com_hdl.dev_hdl, &dev_attr);
    if(result) {
	fprintf( stderr, "%s : eia232dev_get_attr fail, %s\n", av[0], strerror(result));
	return result;
    }

    dev_attr.baudrate = 115200;
#if 0
    dev_attr.databits = EIA232DEV_DATA_8;
    dev_attr.parity = EIA232DEV_PAR_NONE;
    dev_attr.stopbits = EIA232DEV_STOP_1;
    dev_attr.flowctrl = EIA232DEV_FLOW_OFF;
#endif
    dev_attr.timeout_msec = 1000;

    result = s3::mcxx_dev232_set_attr( &com_hdl.dev_hdl, &dev_attr);
    if(result) {
	fprintf( stderr, "%s : eia232dev_set_attr fail, %s\n", av[0], strerror(result));
	return result;
    }

    /* エコー出力停止コマンドを二回発行 */
    result = s3::mcxx_dev232_write_string( &com_hdl.dev_hdl, logoff_str, &retlen);
    if(result) {
	fprintf( stderr, "%s : cmd %s fail\n", av[0], logoff_str);
    }
    result = s3::mcxx_dev232_write_string( &com_hdl.dev_hdl, logoff_str, &retlen);
    if(result) {
	fprintf( stderr, "%s : cmd %s fail\n", av[0], logoff_str);
    }

    /* リアルタイムログ停止コマンドを発行 */
    result = s3::mcxx_dev232_write_string( &com_hdl.dev_hdl, echooff_str, &retlen);
    if(result) {
	fprintf( stderr, "%s : cmd %s fail\n", av[0], echooff_str);
	return result;
    }

    /* 受信情報はすべて破棄 */
    result = s3::mcxx_dev232_clear_recv( &com_hdl.dev_hdl);
    if(result) {
	return result;
    }

    return 0;
}

static int close_cmd(int ac, char **av) {
    if(!com_hdl.init.f.dev_hdl) {
	return EIO;
    }

    s3::mcxx_dev232_close( &com_hdl.dev_hdl );

    com_hdl.init.f.dev_hdl = 0;

    return 0;
}

static int devstat_cmd(int ac, char **av) {
    int result;
    s3::mcxx_dev232_attr_t devattr;

    if(!com_hdl.init.f.dev_hdl) {
	return EIO;
    }

    result = s3::mcxx_dev232_get_attr( &com_hdl.dev_hdl, &devattr);
    if(result) {
	return result;
    }

    fprintf( stdout, "baurate=%d\n", devattr.baudrate);
    fprintf( stdout, "databits = %s\n", s3::mcxx_dev232_databitstype2str(devattr.databits));
    fprintf( stdout, "parity = %s\n",   s3::mcxx_dev232_paritytype2str(devattr.parity));
    fprintf( stdout, "stopbits = %s\n", s3::mcxx_dev232_stopbitstype2str(devattr.stopbits));
    fprintf( stdout, "flowctrl = %s\n", s3::mcxx_dev232_flowtype2str(devattr.flowctrl));
    fprintf( stdout, "timeout_msec = %d\n", devattr.timeout_msec);

    return 0;
}

static int search_cmd(int ac, char **av)
{
    int result;
    int32_t vid, pid;
    s3::mcxx_dev232_find_device_t finddev;
    char devname[MAX_PATH];
    int loop=1;
    vid = pid = ~0;

    if( ac!=3 ) {
	result = -1;
    } else if ( sscanf( av[1], "%i", &vid) != 1 ) {
	result = -1;
    } else if ( sscanf( av[2], "%i", &pid) != 1 ) {
	result = -1;
    } else {
    	result = 0;
    }

    if(result) {
	fprintf( stdout, "%s <0x(vender_id)> <0x(Product_id)>\n", av[0]);
	return 0;
    }

    result = s3::mcxx_dev232_find_device_init(&finddev);
    if(result) {
	fprintf( stdout, "s3::mcxx_dev232_find_device_init() failed\n");
	return EIO;
    }

    while(loop) {
	result = s3::mcxx_dev232_find_device_exec(&finddev, vid, pid, devname, MAX_PATH);
	if(result) {
	    switch(result) {
	    case ENOMEM:
		/* devname の設定するためのリソースがない */
		fprintf( stdout, "devname size is small\n");
		break;
	    case ENODEV:
		fprintf( stdout, "search progress done.\n");
		break;
	    case EIO:
		fprintf( stdout, "search progress failed.\n");
		break;
	    default:
		fprintf( stdout, " %s error\n", strerror(result));
	    }
	    loop = 0;
	} else {
	    fprintf( stdout, "find device name = %s\n", devname);
	}
    }
    s3::mcxx_dev232_find_device_destroy(&finddev);

    return 0;
}

static int command_cmd(int ac, char **av)
{
    int status, result;
    char resbuf[256];
    size_t retval;

    if(!com_hdl.init.f.dev_hdl) {
	return EIO;
    }

    if(ac != 2) {
	status = 1;
    } else {
	status = 0;
    }

    if(status) {
	fprintf( stdout, "%s <command> or \"<commands ...>\"\n", av[0]);
	return 0;
    }

    result = s3::mcxx_dev232_write_string( &com_hdl.dev_hdl, av[1], &retval);
    if(result) {
	return result;
    }

    result = s3::mcxx_dev232_readln_string( &com_hdl.dev_hdl, resbuf, 256, &retval);
    if(result) {
	return result;
    }

    fprintf(stdout, "%s\n", resbuf);

    return 0; 
}


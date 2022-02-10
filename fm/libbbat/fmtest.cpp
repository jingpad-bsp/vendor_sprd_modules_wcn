#include "autotest.h"
#include "fm.h"
#define FM_START_FREQ 875
unsigned char skd_fm_status_r = 0x02;//0: pass 1: fail
unsigned int skd_fm_freq_r = 0;
int skd_fm_rssi_r = 0;
int bbat_fm_rssi_r = 0;

DYMIC_WRITETOPC_FUNC g_func[WRITE_TO_MAX] = {NULL};

 /* static report_ptr g_func; */
 int fm_bbat_audio(DYMIC_WRITETOPC_FUNC * write_interface_ptr)
 {
	 bool flag = false;
	 LOGD("%s", __FUNCTION__);

	 if (flag == false) {
		 LOGD("fm_bbat at");
		 for(int i = 0; i < WRITE_TO_MAX; i++) {
			 g_func[i] = write_interface_ptr[i];
			 if(g_func[i] != NULL)
				 LOGD("%s ad 0x%x, i %d", __FUNCTION__, g_func[i], i);
		 }
		 flag = true;
	 }
	 return 0;
 }

//int testFM(const uchar * data, int data_len, uchar * rsp, int rsp_size)
int testFM(char *buf, int len, char *rsp, int rsplen)
{
	 int ret = 0;  //用于表示具体测试函数返回值，
	/****************************************************************
    ret <0,表示函数返回失败。
	ret =0,表示测试OK，无需返回数据。
	ret >0,表示测试OK，获取ret个数据，此时，ret的大小表示data中保存的数据的个数。
	***********************************************************************/
/***
FM	PC-->engpc:7E 4B 00 00 00 0C 00 38 0D 01 00 04 38 7E	FM打开	红色字段表示频道信息，04 38表示108.0MHz，FM要求信号从耳机左右声道输出
	engpc-->pc:7E 00 00 00 00 08 00 38 00 7E
	PC-->engpc:7E 4A 00 00 00 09 00 38 0D 02 7E	FM关闭	关闭FM功能
	engpc-->pc:7E 00 00 00 00 08 00 38 00 7E
***/
	char data[1014] = {0 }; //用于保存获取到的数据。

	FUN_ENTER ; //打印函数提名字：testGpio ++
	//打印PC下发的数据。
	LOGD("pc->engpc:%d number--> %x %x %x %x %x %x %x %x %x %x %x ",len,buf[0],buf[1],buf[2],buf[3],buf[4],buf[5],buf[6],buf[7],buf[8],buf[9],buf[10]); //78 xx xx xx xx _ _38 _ _ 打印返回的10个数据	

    switch( buf[9] ) {
	case 1:
	    {
		uint freq = ((buf[10] << 16) | (buf[11] << 8) | (buf[12] << 0)); //899
		freq *=10;
		LOGD("input freq:%d\n",freq);

		//for android 6 and upon platform Using google original apk  or marlin
#if ((defined( GOOGLE_FM_INCLUDED) || defined (SPRD_WCNBT_MARLIN) )&&(!defined(BOARD_HAVE_FM_BCM)))
		if( Radio_Open(freq) < 0 || Radio_Play(freq) < 0 ){
		    ret = -1;
		    bbat_fm_rssi_r = 0;
		    LOGD("Radio_Open or Radio_Play the fm open failll!!!\n");
		}else{
		    ret =  Radio_GetRssi(&bbat_fm_rssi_r);
		    if(0!=ret){
			ret = -1;
			LOGD("Radio_Open or Radio_Play the fm open failll!!!\n");
		    }else{
			ret = 4;
			data[0]  = bbat_fm_rssi_r >> 24;
			data[1]  = (bbat_fm_rssi_r >> 16)&0xFF;
			data[2]  = (bbat_fm_rssi_r >> 8)&0xFF;
			data[3]  = (bbat_fm_rssi_r >> 0)&0xFF;
			LOGD("-----------data[0] = %d data[1] = %d data[2]= %d data[3] = %d  \n", data[0], data[1], data[2], data[3]);
		}

		}
#else
#ifdef SPRD_WCNBT_SR2351 //2351
		if( fmOpen() < 0 || fmPlay(freq) < 0 ){ ret = -1;}
#else
#ifdef BOARD_HAVE_FM_BCM //BCM

		if( Bcm_fmOpen() < 0 || Bcm_fmPlay(freq,&bbat_fm_rssi_r) < 0 ){
		    //ret = 5;
		    ret = -1;
		LOGD("the fm open failll!!!\n");
		}else{
		    ret = 4;
		    //(unsigned char &)bbat_fm_rssi_r;
		    data[0]  = bbat_fm_rssi_r >> 24;
		    data[1]  = (bbat_fm_rssi_r >> 16)&0xFF;
		    data[2]  = (bbat_fm_rssi_r >> 8)&0xFF;
		    data[3]  = (bbat_fm_rssi_r >> 0)&0xFF;

		    LOGD("-----------data[0] = %d data[1] = %d data[2]= %d data[3] = %d  \n", data[0], data[1], data[2], data[3]);
		}


#endif
#endif
#endif
	    }
	    break;
	case 2:
	    //for android 6 and upon platform Using google original apk marlin
#if ((defined( GOOGLE_FM_INCLUDED) || defined (SPRD_WCNBT_MARLIN))&&(!defined(BOARD_HAVE_FM_BCM)))
	    if( Radio_Close() < 0 )
		ret = -1;
#endif

#ifdef SPRD_WCNBT_SR2351 //2351
	    fmStop();
	    fmClose();
#endif

#ifdef BOARD_HAVE_FM_BCM //bcm
	    Bcm_fmStop();
	    Bcm_fmClose();
#endif
	    break;
	case 0x20:
	    {
		skd_fm_freq_r = FM_START_FREQ;
		ret = Radio_SKD_Test(&skd_fm_status_r, &skd_fm_freq_r, &skd_fm_rssi_r);
		LOGD("status = %d, freq = %d, rssi = %d\n",
			skd_fm_status_r, skd_fm_freq_r, skd_fm_rssi_r);
	    }
	    break;
	default:
	    break;
    }


/*------------------------------后续代码为通用代码，所有模块可以直接复制，------------------------------------------*/

	//填充协议头7E xx xx xx xx 08 00 38
	MSG_HEAD_T *msg_head_ptr;
	memcpy(rsp, buf, 1 + sizeof(MSG_HEAD_T)-1);  //复制7E xx xx xx xx 08 00 38至rsp,，减1是因为返回值中不包含MSG_HEAD_T的subtype（38后面的数据即为subtype）
	msg_head_ptr = (MSG_HEAD_T *)(rsp + 1);

	//修改返回数据的长度，如上复制的数据长度等于PC发送给engpc的数据长度，现在需要改成engpc返回给pc的数据长度。
	msg_head_ptr->len = 8; //返回的最基本数据格式：7E xx xx xx xx 08 00 38 xx 7E，去掉头和尾的7E，共8个数据。如果需要返回数据,则直接在38 XX后面补充


	//填充成功或失败标志位rsp[1 + sizeof(MSG_HEAD_T)]，如果ret>0,则继续填充返回的数据。
	LOGD("msg_head_ptr,ret=%d",ret);

	if(ret<0)
	{
	rsp[sizeof(MSG_HEAD_T)] = 1;    //38 01 表示测试失败。
	}else if (ret==0){
	rsp[sizeof(MSG_HEAD_T)] = 0;    //38 00 表示测试ok
	}else if(ret >0)
	{
	rsp[sizeof(MSG_HEAD_T)] = 0;    //38 00 表示测试ok,ret >0,表示需要返回 ret个字节数据。
    memcpy(rsp + 1 + sizeof(MSG_HEAD_T), data, ret);	  //将获取到的ret个数据，复制到38 00 后面
	msg_head_ptr->len+=ret;   //返回长度：基本数据长度8+获取到的ret个字节数据长度。
	}
	LOGD("rsp[1 + sizeof(MSG_HEAD_T):%d]:%d",sizeof(MSG_HEAD_T),rsp[sizeof(MSG_HEAD_T)]);
	//填充协议尾部的7E
    rsp[msg_head_ptr->len + 2 - 1]=0x7E;  //加上数据尾标志
    LOGD("dylib test :return len:%d",msg_head_ptr->len + 2);
	LOGD("engpc->pc:%x %x %x %x %x %x %x %x %x %x",rsp[0],rsp[1],rsp[2],rsp[3],rsp[4],rsp[5],rsp[6],rsp[7],rsp[8],rsp[9]); //78 xx xx xx xx _ _38 _ _ 打印返回的10个数据
	FUN_EXIT ; //打印函数体名字
   return msg_head_ptr->len + 2;
/*------------------------------如上虚线之间代码为通用代码，直接赋值即可---------*/
}

int read_test_result(char *buf, int len, char *rsp, int rsplen)
{
	FUN_ENTER;
	char data[1014] = {0 }; //用于保存获取到的数据。
	int ret = ENG_DIAG_RET_UNSUPPORT;
	LOGD("read_test_result pc->engpc:%d number--> %x %x %x %x %x %x %x %x %x %x %x ",len,buf[0],buf[1],buf[2],buf[3],buf[4],buf[5],buf[6],buf[7],buf[8],buf[9],buf[10]);
    ALOGD("buf[9] = 0x%02x\n", buf[9]);

	if (buf[9] == 0x15){
        if (skd_fm_status_r == 0) {
            ret = 7;
            data[0]= skd_fm_status_r;
            data[1]= skd_fm_freq_r>>8;
            data[2]= skd_fm_freq_r>>0;
            data[3]= skd_fm_rssi_r>>24;
            data[4]= skd_fm_rssi_r>>16;
            data[5]= skd_fm_rssi_r>>8;
            data[6]= skd_fm_rssi_r>>0;
        } else {
            ret = 1;
            data[0]= skd_fm_status_r;
        }
	}

	//填充协议头7E xx xx xx xx 08 00 38
	MSG_HEAD_T *msg_head_ptr;
	memcpy(rsp, buf, 1 + sizeof(MSG_HEAD_T)-1);  //复制7E xx xx xx xx 08 00 38至rsp,，减1是因为返回值中不包含MSG_HEAD_T的subtype（38后面的数据即为subtype）
	msg_head_ptr = (MSG_HEAD_T *)(rsp + 1);

	//修改返回数据的长度，如上复制的数据长度等于PC发送给engpc的数据长度，现在需要改成engpc返回给pc的数据长度。
	msg_head_ptr->len = 8; //返回的最基本数据格式：7E xx xx xx xx 08 00 38 xx 7E，去掉头和尾的7E，共8个数据。如果需要返回数据,则直接在38 XX后面补充

	//填充成功或失败标志位rsp[1 + sizeof(MSG_HEAD_T)]，如果ret>0,则继续填充返回的数据。
	LOGD("msg_head_ptr,ret=%d",ret);

	if(ret<0) {
		rsp[sizeof(MSG_HEAD_T)] = 1;    //38 01 表示测试失败。
	}else if(ret >0) {
		rsp[sizeof(MSG_HEAD_T)] = 0;    //38 00 表示测试ok,ret >0,表示需要返回 ret个字节数据。
		memcpy(rsp + 1 + sizeof(MSG_HEAD_T), data, ret);	  //将获取到的ret个数据，复制到38 00 后面
		msg_head_ptr->len+=ret;   //返回长度：基本数据长度8+获取到的ret个字节数据长度。
	}
	LOGD("rsp[1 + sizeof(MSG_HEAD_T):%d]:%d",sizeof(MSG_HEAD_T),rsp[sizeof(MSG_HEAD_T)]);
	//填充协议尾部的7E
    rsp[msg_head_ptr->len + 2 - 1]=0x7E;  //加上数据尾标志
    LOGD("dylib test :return len:%d",msg_head_ptr->len + 2);
	LOGD("engpc->pc:%x %x %x %x %x %x %x %x %x %x",rsp[0],rsp[1],rsp[2],rsp[3],rsp[4],rsp[5],rsp[6],rsp[7],rsp[8],rsp[9]); //78 xx xx xx xx _ _38 _ _ 打印返回的10个数据
	FUN_EXIT ; //打印函数体名字
    return msg_head_ptr->len + 2;
}

extern "C" void register_this_module_ext(struct eng_callback *reg, int *num)
{
   int moudles_num = 0;   //用于表示注册的节点的数目。
   LOGD("register_this_module :dllibtest");

    //1st command
   reg->type = 0x38;  //38对应BBAT测试
   reg->subtype = 0x0D;   //0x0D对应BBAT中的FM测试。
   reg->eng_diag_func = testFM; //testGpio对应BBAT中GPIO测试功能函数，见上。
   reg->eng_set_writeinterface_func = fm_bbat_audio;
   moudles_num++;

   /*************
   //2st command
   reg->type = 0x39;  //39对应nativemmi测试
   reg->subtype = 0x07;   //0x07对应nativemmi中的GPIO测试。
   reg->eng_diag_func = testnativeGpio;
   moudles_num++;
   **********************/

   (reg + moudles_num)->type = 0x38;
   (reg + moudles_num)->subtype = 0x03;
   (reg + moudles_num)->eng_diag_func = read_test_result;
   moudles_num++;

   *num = moudles_num;
   LOGD("register_this_module_ext: %d - %d",*num, moudles_num);
}

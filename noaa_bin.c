////////////////////////////////////////////////////////
//                                                    //
//  NOAA APT signal Encoder by Borland C++ 7.30       //
//                                  Ver0.50           //
//                                                    //
//                        Presented by Yukio Ohata    //
//                                         2021.01.01 //
////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define  MEM_SIZE 44100                                     // Size of voice memory
#define  MEM_SIZE_iq  44100
                                                            // Terlemetry Frame
#define  WDG_1 31                                           // WEDGE #1
#define  WDG_2 63                                           // WEDGE #2
#define  WDG_3 95                                           // WEDGE #3
#define  WDG_4 127                                          // WEDGE #4
#define  WDG_5 159                                          // WEDGE #5
#define  WDG_6 191                                          // WEDGE #6
#define  WDG_7 223                                          // WEDGE #7
#define  WDG_8 255                                          // WEDGE #8
#define  ZERO_MOD_WDG 0                                     // zero Modulation
#define  THM_TMP_1 55                                       // Thermister Temp #1
#define  THM_TMP_2 55                                       // Thermister Temp #2
#define  THM_TMP_3 55                                       // Thermister Temp #3
#define  THM_TMP_4 55                                       // Thermister Temp #4
#define  PTC_TMP 125                                        // Patch Temp
#define  BAC_SCN 1                                          // Back Scan
#define  CH_ID_WDG 30                                       // Channel I.D. Wedge

int main(int argc, char **argv){

	//APT format
	unsigned char sync_a[39] = {0,0,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,0,0,0,0,0,0,0};
	unsigned char image_a[909];
	unsigned char telemetry_a[45];

	unsigned char sync_b[39] = {0,0,0,0,1,1,1,0,0,1,1,1,0,0,1,1,1,0,0,1,1,1,0,0,1,1,1,0,0,1,1,1,0,0,1,1,1,0,0};
	unsigned char image_b[909];
	unsigned char telemetry_b[45];
	
	unsigned char telemetry[16] = {WDG_1,WDG_2,WDG_3,WDG_4,WDG_5,WDG_6,WDG_7,WDG_8,ZERO_MOD_WDG,THM_TMP_1,THM_TMP_2,THM_TMP_3,THM_TMP_4,PTC_TMP,BAC_SCN,CH_ID_WDG};
	double line_pic[2080];

	int bmp_wid;
	int bmp_hgt;
	int bmp_bitcount;

	//Vars for input bmt file output wav file and output bin(I/Q) file
	FILE *f1, *f2, *f3;
	unsigned char  tmp1;                                    // 1Byte
	unsigned short tmp2;                                    // 2Byte
	unsigned long  tmp4;                                    // 4Byte
	unsigned short channel;                                 // number of output channel
	unsigned short BytePerSample;                           // Byte number per 1 sample
	unsigned long  file_size;                               // output file size
	unsigned long  Fs_out;                                  // sampling frequency of output wav file
	unsigned long  BytePerSec;                              // Byte number per 1 second
	unsigned long  data_len;                                // 出力波形のサンプル数
	unsigned long  fmt_chnk    =16;
	unsigned short BitPerSample=8;
	unsigned short fmt_ID      =1;                          // fmt ID.

	if(argc != 4){
		fprintf( stderr, "Usage: noaa_bin.exe input.bmp output.wav output.bin \n" );
		exit(-1);
	}
	if((f1 = fopen(argv[1], "rb")) == NULL){
		fprintf( stderr, "Cannot open %s\n", argv[1] );
		exit(-2);
	}
	if((f2 = fopen(argv[2], "wb")) == NULL){
		fprintf( stderr, "Cannot open %s\n", argv[2] );
		exit(-2);
	}

	//read input bmp file header
	fseek(f1, 18L, SEEK_SET);
	fread(&tmp4, sizeof(unsigned long), 1, f1);
	bmp_wid = tmp4;
	fread(&tmp4, sizeof(unsigned long), 1, f1);
	bmp_hgt = tmp4;
	fseek(f1, 28L, SEEK_SET);
	fread(&tmp2, sizeof(unsigned long), 1, f1);
	bmp_bitcount = tmp2;

	if(bmp_wid != 909){
		fprintf( stderr, "the width of input bmp file must be 909 pixel\n");
		exit(-2);
	}
	if(bmp_hgt > 2400){
		fprintf( stderr, "the height of input bmp file should be under 2400 pixel\n");
		exit(-2);
	}
	if(bmp_bitcount != 24){
		fprintf( stderr, "the bitcount of input bmp file must be 24bit color\n");
		exit(-2);
	}
	
	//信号処理用変数
	int           t             = 0;                                  // 時刻の変数
	int           n             = 0;                                  //
	int           k             = 0;                                  //
	int           m             = 0;
	int           i             = 0;
	long int      t_out         = 0;                                  // 終了時刻計測用の変数
	int           add_len       = 0;                                  // 出力信号を延長するサンプル数
	unsigned char input, output;                                      // 読込み変数と書出し変数
	double        s[MEM_SIZE+1] = {0};                                // 入力データ格納用変数
	int           Fs            = 44100;                              // sampling frequency of output wav file
	int           ch            = 1;                                  // 入力チャネル数
	int           len           = 0.5*Fs*bmp_hgt;
	double        f             = 2400.0;

	//出力waveファイルの準備
	Fs_out        = Fs;                                               // 出力サンプリング周波数を設定
	channel       = ch;                                               // 出力チャネル数を設定
	data_len      = channel*(len+add_len)*BitPerSample/8;             // 出力データの長さ = 全Byte数 (1サンプルで2Byte)
	file_size     = 36+data_len;                                      // 全体ファイルサイズ
	BytePerSec    = Fs_out*channel*BitPerSample/8;                    // 1秒当たりのバイト数
	BytePerSample = channel*BitPerSample/8;                           // 1サンプル当たりのバイト数

	//出力ヘッダ情報書き込み
	fprintf(f2, "RIFF");                                              // "RIFF"
	fwrite(&file_size,    sizeof(unsigned long ), 1, f2);             // ファイルサイズ
	fprintf(f2, "WAVEfmt ");                                          // "WAVEfmt"
	fwrite(&fmt_chnk,     sizeof(unsigned long ), 1, f2);             // fmt_chnk=16 (ビット数)
	fwrite(&fmt_ID,       sizeof(unsigned short), 1, f2);             // fmt ID=1 (PCM)
	fwrite(&channel,      sizeof(unsigned short), 1, f2);             // 出力チャネル数
	fwrite(&Fs_out,       sizeof(unsigned long ), 1, f2);             // 出力のサンプリング周波数
	fwrite(&BytePerSec,   sizeof(unsigned long ), 1, f2);             // 1秒当たりのバイト数
	fwrite(&BytePerSample,sizeof(unsigned short ),1, f2);             // 1サンプル当たりのバイト数
	fwrite(&BitPerSample, sizeof(unsigned short ),1, f2);             // 1サンプルのビット数(8ビット)
	fprintf(f2, "data");                                              // "data"
	fwrite(&data_len,     sizeof(unsigned long ), 1, f2);             // length of output wav data

	//main loop
	fseek(f1, 54L, SEEK_SET);
	while(t_out<len){
		if (n==0) {
			for (k=0;k<909;k++){
				i = 0;
				fread(&tmp1, sizeof(unsigned char), 1, f1);
				i = i + tmp1;
				image_a[k] = i;
				fread(&tmp1, sizeof(unsigned char), 1, f1);
				i = i + tmp1;
				fread(&tmp1, sizeof(unsigned char), 1, f1);
				i = i + tmp1;
				image_b[k] = i / 3;
			}
			fread(&tmp1, sizeof(unsigned char), 1, f1);
			for (k=0;k<45;k++){
				telemetry_a[k] = telemetry[(t_out/(8*22050))%16];
				telemetry_b[k] = telemetry[(t_out/(8*22050))%16];
			}
			for (k=0;k<39;k++){
				line_pic[k] = sync_a[k]*255;
				line_pic[k+1040] = sync_b[k]*255;
			}
			for (k=39;k<86;k++){
				if ((t_out/22050)%120 == 14 || (t_out/22050)%120 == 15 ){
					line_pic[k] = 0;
					line_pic[k+1040] = 0;
				} else if ((t_out/22050)%120 == 16 || (t_out/22050)%120 == 17 ){
					line_pic[k] = 255;
					line_pic[k+1040] = 255;
				} else {
					line_pic[k] = 0;
					line_pic[k+1040] = 255;
				}
			}
			for (k=86;k<995;k++){
				line_pic[k] = image_a[k-86];
				line_pic[k+1040] = image_b[k-86];
			}
			for (k=995;k<1040;k++){
				line_pic[k] = telemetry_a[k-995];
				line_pic[k+1040] = telemetry_b[k-995];
			}
		}

		//Signal Processing
		k = 10;                                                                 // adjust the pixcels per 1 line
		if(n%2==1 || n%10==2 || n%1040==4) {                                    // 2080*10   +  1040   +   208    +     2 = 22050
			k = 11;                                                             //           =2080/2  =2080/10 =2080/1040
		}
		for (m=0;m<k;m++) {
			s[t]=0.5+0.5*(sin(2.0*M_PI*f*t_out/Fs_out)*(line_pic[n]/255));      // Amplitude Modulation
			output = s[t]*255;                                                  // 出力を整数化
			fwrite(&output, sizeof(unsigned char), 1, f2);                      // 結果の書き出し
			t=(t+1)%MEM_SIZE;                                                   // 時刻 t の更新
			t_out++;                                                            // ループ終了時刻の計測
		}
		n=(n+1)%2080;
	}
	fclose(f1);
	fclose(f2);
	
	printf("\n%s is successfully made\n",argv[2]);
	
	if((f2 = fopen(argv[2], "rb")) == NULL){
		fprintf( stderr, "Cannot open %s\n", argv[2] );
		exit(-2);
	}
	if((f3 = fopen(argv[3], "wb")) == NULL){
		fprintf( stderr, "Cannot open %s\n", argv[3] );
		exit(-2);
	}
	
	fseek(f2, 40L, SEEK_SET);
	fread(&tmp4, sizeof(unsigned long), 1, f2);
	
	double    F15     = 137620000.0;                                    // NOAA15 freq
	double    Fs_iq   = 44100*64.0;                                     // 44100*50
	double    theta   = 0.0;                                            // 変調用位相
	double    mr      = 25000.0/4800.0;                                 // 0.25 115k 0.75 120k 0.83 130k 0.95 160k
	double    x_FM;                                                     // FM変調信号
	double    tmp[2];
	unsigned  tmp_un;
	signed char xI;
	signed char xQ;
	
	fseek(f2, 44L, SEEK_SET);
	tmp[0] = 0.0; 
	for (i=0;i<22050*2*10;i++){
		fread(&tmp_un,sizeof(unsigned char), 1, f2);
		
		tmp[1] = tmp[0];
		tmp[0] = (tmp_un-127.5)/128;
		
		for (m=0;m<16;m++){
			for (k=0;k<4;k++){
				theta = theta + (tmp[1]*(m+1.0)+tmp[0]*(15.0-m))/16.0;
				if( theta > M_PI ){
					theta = theta - 2.0*M_PI;
				}
				if( theta <-M_PI ){
					theta = theta + 2.0*M_PI;
				}
				x_FM = cos(2*M_PI*F15*t/Fs_iq + mr*theta);
				xI = (signed char)((x_FM *   cos(2.0*M_PI*F15*t/Fs_iq + mr*theta)) * 127.0);
				xQ = (signed char)((x_FM * (-sin(2.0*M_PI*F15*t/Fs_iq + mr*theta)))* 127.0);
				fwrite(&xI, sizeof(signed char), 1, f3);
				fwrite(&xQ, sizeof(signed char), 1, f3);
				t=(t+1)%MEM_SIZE_iq; 
			}
		}
	}
	printf("\n%s is successfully made\n",argv[3]);
	fclose(f2);
	fclose(f3);

	return 0;
}

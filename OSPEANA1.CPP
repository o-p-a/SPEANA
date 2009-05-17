/*
		スペアナくん１号

		オーバーレイしてよい部分。

		By Ken/ichiro(OPA)
*/

#include "stdincl.h"
#pragma hdrstop
#define GVAR_DEF
#include "opa.h"
#include "ovrstr.h"
#include "mainlib.h"
#include "opaenv.h"
#include "opaarg.h"
#include "filename.h"
#include "istrm.h"
#include "ostrm.h"
#include "kbd.h"
#include "putget.h"
#include "opicload.h"
#include "speana.h"

#pragma option -zEOSPEANA1_TEXT -zFCODE

#define VERSTR			"1.16"

extern uint
	_stklen,			// Byte (default:4112)
	_ovrbuffer;			// Para (default:0)

static EmsExtMemPool
	ePool(PGM);
static MainExtMemPool
	mPool;
static GvramSave
	gSave;
static PaletteSave
	pSave;
static TtBios
	tt;
StdOut
	cout;
StdErr
	cerr;
static MhwArray
	mhw;
static sint
	tFreq=400;
static bool
	thru=false,
	syncOut=false,
	sendFe=true,
	midiSync=false,
	autoTempo=false,
	multiPort=false;

static uchar
	myName[MAXPATH],
	playFile[MAXPATH];
static bool
	error=false;
static sint
	rcode=0;

static schar
	sinTab[Spc::BARS]={
53,44,34,23,11,-1,-14,-26,-37,-46,-54,-60,-64,-64,-63,-59,-53,-44,-34,-23,-11,1,14,26,37,46,54,60,64,64,63,59
	},
	cosTab[Spc::BARS]={
-37,-46,-54,-60,-64,-64,-63,-59,-53,-44,-34,-23,-11,1,14,26,37,46,54,60,64,64,63,59,53,44,34,23,11,-1,-14,-26
	};
		// ↑三角関数テーブル。あとで計算が楽なように徹底的に最適化して
		// あります。ただしふたつのテーブルを結合することはしていません。
		// awkスクリプトは以下の通り。
/*	sincos.awk
BEGIN{
	PI=3.1415926535;

	BARS=32;
	RADIUS=65;
	AR=125;

	for(i=0;i<BARS;++i){
		v=RADIUS*sin(AR*PI/180+2*PI/BARS*i);
		if(i!=0)
			printf(",");
		printf("%d",v);
	}

	printf("\n");

	for(i=0;i<BARS;++i){
		v=RADIUS*cos(AR*PI/180+2*PI/BARS*i);
		if(i!=0)
			printf(",");
		printf("%d",v);
	}
}
*/

///////////////////////////////////////////////////////////////////////////////

void Level::init()
{
	_mvol=127;

	for(sint i=0;i<PARTS;++i){
		_vol[i]=100;
		_exp[i]=127;
		setTtl(i);
	}
}

///////////////////////////////////////////////////////////////////////////////

void Face::live()							// 顔は生きて（起きて）ます！
{
	_state=LIVE;
	_eyeBlink=0;
	_eyePos=2;
	_eyeFlag=false;
	_mouth=0;
	_mouthFlag=false;
	_noteCount=0;
}

void Face::dead()						// 顔は死んでます（骸骨の表示は省略）
{
	_state=DEAD;
}

void Face::pos(sint x,sint y)
{
	_x=x;
	_y=y;
}

void Face::allSleep()						// なんかスコープが目茶苦茶だぞ(笑)
{
	for(sint i=0;i<FACES;++i)
		if(face[i]._state==LIVE)
			face[i].sleep();
}

void Face::drawPupil(sint t,sint b)			// 瞳を描きます
{
	gv.boxFill(_x+ 3,_y+t,_x+ 8,_y+b,15);
	gv.boxFill(_x+ 3+_eyePos,_y+t,_x+ 4+_eyePos,_y+b,10);
	gv.boxFill(_x+11,_y+t,_x+16,_y+b,15);
	gv.boxFill(_x+11+_eyePos,_y+t,_x+12+_eyePos,_y+b,10);
}

void Face::drawEye()						// 目玉ｷｮﾛｷｮﾛを描きます
{
	_eyeFlag=false;

	switch(_state){
	case SLEEP:
		_eyeBlink=5;
		// fall...
	case LIVE:
		gv.boxFill(_x+3,_y+4,_x+16,_y+9,12);
		switch(_eyeBlink){
		case 0:
			gv.boxFill(_x+3,_y+5,_x+8,_y+8,15);
			gv.line(_x+4,_y+4,_x+7,_y+4,15);
			gv.line(_x+4,_y+9,_x+7,_y+9,15);

			gv.boxFill(_x+11,_y+5,_x+16,_y+8,15);
			gv.line(_x+12,_y+4,_x+15,_y+4,15);
			gv.line(_x+12,_y+9,_x+15,_y+9,15);

			drawPupil(6,7);
		nCase 1:
			gv.boxFill(_x+3,_y+6,_x+8,_y+8,15);
			gv.line(_x+4,_y+5,_x+7,_y+5,15);
			gv.line(_x+4,_y+9,_x+7,_y+9,15);

			gv.boxFill(_x+11,_y+6,_x+16,_y+8,15);
			gv.line(_x+12,_y+5,_x+15,_y+5,15);
			gv.line(_x+12,_y+9,_x+15,_y+9,15);

			drawPupil(7,8);
		nCase 2:
			gv.boxFill(_x+3,_y+7,_x+8,_y+8,15);
			gv.line(_x+4,_y+6,_x+7,_y+6,15);
			gv.line(_x+4,_y+9,_x+7,_y+9,15);

			gv.boxFill(_x+11,_y+7,_x+16,_y+8,15);
			gv.line(_x+12,_y+6,_x+15,_y+6,15);
			gv.line(_x+12,_y+9,_x+15,_y+9,15);

			drawPupil(7,8);
		nCase 3:
			gv.line(_x+4,_y+7,_x+7,_y+7,15);
			gv.line(_x+3,_y+8,_x+8,_y+8,15);
			gv.line(_x+4,_y+9,_x+7,_y+9,15);

			gv.line(_x+12,_y+7,_x+15,_y+7,15);
			gv.line(_x+11,_y+8,_x+16,_y+8,15);
			gv.line(_x+12,_y+9,_x+15,_y+9,15);

			drawPupil(8,8);
		nCase 4 or 5:
			gv.line(_x+3,_y+8,_x+8,_y+8,10);
			gv.line(_x+11,_y+8,_x+16,_y+8,10);
		}
	nCase DEAD:								// でも死んでたら描きません
		;
	}
}

void Face::drawMouth()						// 口ﾊﾟｸﾊﾟｸを描きます
{
	_mouthFlag=false;

	if(_state!=DEAD){
		gv.boxFill(_x+7,_y+10,_x+12,_y+22,12);

		gv.boxFill(_x+8,_y+21-_mouth,_x+11,_y+21,11);
		gv.line(_x+7,_y+21-_mouth+1,_x+7,_y+21-1,11);
		gv.line(_x+12,_y+21-_mouth+1,_x+12,_y+21-1,11);
	}
}

void Face::draw()							// 顔全部を描きます
{
	switch(_state){
	case SLEEP:
		_eyeBlink=5;
		// fall...
	case LIVE:
		gv.boxFill(_x+3,_y+3,_x+20,_y+25,9);
		gv.box(_x+5,_y+2,_x+18,_y+26,9);
		gv.box(_x+2,_y+5,_x+21,_y+23,9);

		gv.boxFill(_x+1,_y+1,_x+18,_y+23,12);
		gv.box(_x+3,_y+0,_x+16,_y+24,12);
		gv.box(_x+0,_y+3,_x+19,_y+21,12);

		drawEye();
		drawMouth();
	nCase DEAD:
		;
	}
}

void Face::update()						// 顔の変化のあった部分だけを描きます
{
	switch(_state){
	case SLEEP:
		_eyeBlink=5;
		// fall...
	case LIVE:
		if(_eyeFlag)
			drawEye();
		if(_mouthFlag)
			drawMouth();
	nCase DEAD:
		;
	}
}

///////////////////////////////////////////////////////////////////////////////

void Lcd::init()
{
	for(sint i=0;i<16;++i){
		_dat[i]=0;
		_mv[i]=true;
	}
	_pict=0;
}

void Lcd::pos(sint x,sint y)
{
	_x=x;
	_y=y;
}

void Lcd::draw(sint i)						// LCD縦一列を描きます
{
	uint
		mask=0x8000U;
	sint
		j,
		x=_x+8*i,
		y=_y;

	_mv[i]=false;
	if(_pict<=0){
		mask = 0xffffU >> (15-_dat[i]/8);
		if(mask==0)
			mask=1;

		for(j=0;j<16;++j){
			gv.boxFill(x,y,x+6,y+2,(mask & 0x8000U)?13:9);
			mask<<=1;
			y+=4;
		}
	}else{
		for(j=0;j<16;++j){
			gv.boxFill(x,y,x+6,y+2,(_dat[i] & mask)?13:9);
			mask>>=1;
			y+=4;
		}
	}
}

void Lcd::draw()
{
	sint
		x;

	for(x=0;x<16;++x)
		draw(x);
}

void Lcd::update()
{
	sint
		x;

	for(x=0;x<16;++x)
		if(_mv[x])
			draw(x);
}

///////////////////////////////////////////////////////////////////////////////

void Letter::init()
{
	_flag=false;

	clear();
}

void Letter::pos(sint x,sint y)
{
	_x=x;
	_y=y;
}

void Letter::draw()							// 文字を表示します
{
	uchar
		w[16+1];

	_flag=false;

	memcpy(w,_buf,16);
	w[16]='\0';

	Ghar::BCSpsXy(_x,_y,w,14,8,0,14);
}

void Letter::update()
{
	if(_flag)
		draw();
}

///////////////////////////////////////////////////////////////////////////////

void Spa::init()
{
	for(sint b=0;b<BARS;++b){
		_bar[b]=0;
		_ph[b]=0;
		_pht[b]=0;
		_bm[b]=false;
	}
}

void Spa::pos(sint x,sint y)
{
	_x=x;
	_y=y;
}

void Spa::drawBar(sint b)					// スペアナのバー１個を描きます
{
	_bm[b]=false;

	sint
		x=_x+RBARW*b,
		sy=_y,
		ey=_y+RHEIGHT,
		my=ey-mltdiv45(_bar[b],RHEIGHT,HEIGHT),
		py=ey-mltdiv45(_ph[b],RHEIGHT,HEIGHT);
#if 0
	if(py>sy)
		gv.boxFill(x,sy,x+RBARW-2,py-1,0,14);
	if(my>py)
		gv.line(x,py,x+RBARW-2,py,2,14);
	if(my>py+1)
		gv.boxFill(x,py+1,x+RBARW-2,my-1,0,14);
	gv.boxFill(x,my,x+RBARW-2,ey,2,14);
#else
	if(ey>my)
		gv.boxFill(x,my+1,x+RBARW-2,ey,2,14);
	if(my>py)
		gv.boxFill(x,py+1,x+RBARW-2,my,0,14);
	gv.line(x,py,x+RBARW-2,py,2,14);
	if(py>sy)
		gv.boxFill(x,sy,x+RBARW-2,py-1,0,14);
#endif
}

void Spa::draw()
{
	for(sint b=0;b<BARS;++b)
		drawBar(b);
}

void Spa::update()
{
	for(sint b=0;b<BARS;++b)
		if(_bm[b])
			drawBar(b);
}

///////////////////////////////////////////////////////////////////////////////

void Spb::init()
{
	for(sint b=0;b<PARTS;++b){
		_bar[b]=0;
		_pan[b]=64;
		__pan[b]=64;
		_bm[b]=false;
	}
}

void Spb::pos(sint x,sint y)
{
	_x=x;
	_y=y;
}

void Spb::drawBar(sint b)					// Panpot/Touchバー１個を描きます
{
	_bm[b]=false;

	sint
		sx=_x,
		ex=_x+WIDTH,
		lx,
		rx,
		y=_y+RBARH*b;

	lx=_x+mltdiv45(WIDTH-1-_bar[b],__pan[b],127);
	rx=lx+_bar[b]+1;	// いま！割り込みがかかるとバーがはみ出すことがあるので
	maxLimit(rx,ex);	// そうなったときは切り捨てます。

	if(lx>sx)
		gv.boxFill(sx,y,lx-1,y+RBARH-2,0,14);
	gv.boxFill(lx,y,rx,y+RBARH-2,4,14);
	if(ex>rx)
		gv.boxFill(rx+1,y,ex,y+RBARH-2,0,14);
}

void Spb::draw()
{
	for(sint b=0;b<PARTS;++b)
		drawBar(b);
}

void Spb::update()
{
	for(sint b=0;b<PARTS;++b)
		if(_bm[b])
			drawBar(b);
}

///////////////////////////////////////////////////////////////////////////////

void Spc::init()
{
	_sweep=0;
	_ph=0;
	__ph=0;

	for(sint b=0;b<BARS;++b){
		_bar[b]=0;
		_bm[b]=false;
	}
}

void Spc::pos(sint x,sint y)
{
	_x=x;
	_y=y;
}

void Spc::pos2(sint x,sint y)
{
	_x2=x;
	_y2=y;

	for(sint b=0;b<BARS;++b){
		_ox[b]=x;
		_oy[b]=y;
	}
}

void Spc::timeBase(sint tb)
{
	_tb=tb*2;

	reset();
}

void Spc::drawCirc(sint r)					// 説明が難しい(笑)
											// 「ベクトルスコープ」「レーダー
											// チャート」などの単語を連想しつつ
											// いろいろいじった結果できたもの
											// を描きます(笑)
{
	sint
		r1,
		x1,
		y1,
		r2,
		x2,
		y2,
		ox,
		oy,
		nx,
		ny;

	r1=(r+BARS-1)%BARS;
	r2=(r+1)%BARS;

	x1=_ox[r1];y1=_oy[r1];
	x2=_ox[r2];y2=_oy[r2];
	ox=_ox[r] ;oy=_oy[r] ;

	nx=_x2+mltdiv45(cosTab[r],_bar[r],180);
	ny=_y2-mltdiv45(sinTab[r],_bar[r],180);
	_ox[r]=nx;_oy[r]=ny;

	gv.line(x1,y1,ox,oy,0,14);
	gv.line(ox,oy,x2,y2,0,14);
	gv.line(x1,y1,nx,ny,6,14);
	gv.line(nx,ny,x2,y2,6,14);
}

void Spc::drawBar(sint b)					// 「オシロスコープ」を連想しつつ
											// いろいろいじった結果できたもの
											// を描きます。
											// 上のと座標系が違うだけなんだが
{
	_bm[b]=false;

	sint
		x=_x+(WIDTH/BARS)*b,
		sy=_y,
		ey=_y+HEIGHT/4,
		my1=_y+(HEIGHT/8)-_bar[b]/4,
		my2=my1+_bar[b]/2;

	if(my1>sy)
		gv.boxFill(x,sy,x+(WIDTH/BARS)-1,my1-1,0,14);
	if(ey>my2)
		gv.boxFill(x,my2+1,x+(WIDTH/BARS)-1,ey,0,14);
	gv.boxFill(x,my1,x+(WIDTH/BARS)-1,my2,6,14);
}

void Spc::draw()
{
	for(sint b=0;b<BARS;++b){
		drawCirc(b);
		drawBar(b);
	}
}

void Spc::update()
{
	for(sint b=0;b<BARS;++b)
		if(_bm[b]){
			drawCirc(b);
			drawBar(b);
		}
}

///////////////////////////////////////////////////////////////////////////////

static bool poolErr(_ExtMem *,slong,slong)
{
	if(ePool.resize(ePool.size()+16*1024L))
		return true;

	if(mPool.resize(mPool.size()+16*1024L))
		return true;

	return false;
}

#define STDOPT_MHW
#define STDOPT_THW
#define STDOPT_MIDIOPT
#define STDOPT_PAL

enum Option{
	OPT_HELP,
	OPT_VERSION,
#	define STDOPT_ENUM
#	include "stdopt.h"
	OPT_NOA,
	OPT_NOB,
	OPT_NOC,
};

OpaArg::Table
	optionTable[]={
	{OPT_HELP,		"help",		"",			"Show this message" },
	{OPT_HELP,		"?",		"",			"" },
	{OPT_VERSION,	"version",	"",			"Show version" },
#	define STDOPT_OTB
#	include "stdopt.h"
	{"noa"			, OPT_NOA},
	{"nob"			, OPT_NOB},
	{"noc"			, OPT_NOC},
	{0,0,0,0} };

class AppArg : public OpaArg {
	typedef OpaArg Base;
protected:
	virtual void token(sint id,cuchar *s);
public:
	AppArg()		: OpaArg(optionTable) {}
	void version();
	void help();
};

void AppArg::version()
{
	cout << credit << endl;
}

void AppArg::help()
{
	cout<< credit << endl
		<< endl
		<< " くだらないシリーズ  その９" << endl
		<< " 「スペアナくん１号」" << endl
		<< endl
		<< "Usage:" << endl
		<< " " PGM " [/option|file]..." << endl
		<< endl
		<< helpOpt();
}

void AppArg::token(sint optionID,uchar *s)
{
	switch(optionID){
	default or UNKNOWN:
		cerr << PGM_ERR "unknown option " << s << endl;
		error=true;
		rcode=100;
	nCase AMBIGUOUS:
		cerr << PGM_ERR "ambiguous option " << s << endl;
		error=true;
		rcode=101;
	nCase FILE:
		playFile=s;

	nCase OPT_HELP:		help();
	nCase OPT_VERSION:	version();

	nCase OPT_SMPU_1:	mhw.set(M_SMPU_1A,M_SMPU_1B);		multiPort=true;
	nCase OPT_SMPU_1A:	mhw.set(M_SMPU_1A);
	nCase OPT_SMPU_1B:	mhw.set(M_SMPU_1B);
	nCase OPT_SMPU_2:	mhw.set(M_SMPU_2A,M_SMPU_2B);		multiPort=true;
	nCase OPT_SMPU_2A:	mhw.set(M_SMPU_2A);
	nCase OPT_SMPU_2B:	mhw.set(M_SMPU_2B);
	nCase OPT_SMPU_3:	mhw.set(M_SMPU_3A,M_SMPU_3B);		multiPort=true;
	nCase OPT_SMPU_3A:	mhw.set(M_SMPU_3A);
	nCase OPT_SMPU_3B:	mhw.set(M_SMPU_3B);
	nCase OPT_SMPU_4:	mhw.set(M_SMPU_4A,M_SMPU_4B);		multiPort=true;
	nCase OPT_SMPU_4A:	mhw.set(M_SMPU_4A);
	nCase OPT_SMPU_4B:	mhw.set(M_SMPU_4B);

	nCase OPT_MPU_1:	mhw.set(M_MPU_1);
	nCase OPT_MPU_2:	mhw.set(M_MPU_2);
	nCase OPT_MPU_3:	mhw.set(M_MPU_3);
	nCase OPT_MPU_4:	mhw.set(M_MPU_4);

	nCase OPT_SB16_1:	mhw.set(M_SB16_1);
	nCase OPT_SB16_2:	mhw.set(M_SB16_2);

	nCase OPT_MTP_1:	mhw.set(M_MTP_1A,M_MTP_1B,M_MTP_1C,M_MTP_1D);
						mhw.set(M_MTP_1A,M_MTP_1B,M_MTP_1C,M_MTP_1D);
															multiPort=true;
	nCase OPT_MTP_1A:	mhw.set(M_MTP_1A);
	nCase OPT_MTP_1B:	mhw.set(M_MTP_1B);
	nCase OPT_MTP_1C:	mhw.set(M_MTP_1C);
	nCase OPT_MTP_1D:	mhw.set(M_MTP_1D);
	nCase OPT_MTP_1E:	mhw.set(M_MTP_1E);
	nCase OPT_MTP_1F:	mhw.set(M_MTP_1F);
	nCase OPT_MTP_1G:	mhw.set(M_MTP_1G);
	nCase OPT_MTP_1H:	mhw.set(M_MTP_1H);
	nCase OPT_MTP_2A:	mhw.set(M_MTP_2A);
	nCase OPT_MTP_2B:	mhw.set(M_MTP_2B);
	nCase OPT_MTP_2C:	mhw.set(M_MTP_2C);
	nCase OPT_MTP_2D:	mhw.set(M_MTP_2D);
	nCase OPT_MTP_2E:	mhw.set(M_MTP_2E);
	nCase OPT_MTP_2F:	mhw.set(M_MTP_2F);
	nCase OPT_MTP_2G:	mhw.set(M_MTP_2G);
	nCase OPT_MTP_2H:	mhw.set(M_MTP_2H);
	nCase OPT_MTP_3A:	mhw.set(M_MTP_3A);

	nCase OPT_FLYER_1:	mhw.set(M_FLYER_1A,M_FLYER_1B);		multiPort=true;
	nCase OPT_FLYER_1A:	mhw.set(M_FLYER_1A);
	nCase OPT_FLYER_1B:	mhw.set(M_FLYER_1B);

	nCase OPT_RS32:		mhw.set(M_RS_1A,M_RS_1B); 			multiPort=true;
	nCase OPT_RS_1:		mhw.set(M_RS_1A,M_RS_1B,M_RS_1C,M_RS_1D);
						mhw.set(M_RS_1E,M_RS_1F,M_RS_1G,M_RS_1H);
															multiPort=true;
	nCase OPT_RS_1A:	mhw.set(M_RS_1A);
	nCase OPT_RS_1B:	mhw.set(M_RS_1B);
	nCase OPT_RS_1C:	mhw.set(M_RS_1C);
	nCase OPT_RS_1D:	mhw.set(M_RS_1D);
	nCase OPT_RS_1E:	mhw.set(M_RS_1E);
	nCase OPT_RS_1F:	mhw.set(M_RS_1F);
	nCase OPT_RS_1G:	mhw.set(M_RS_1G);
	nCase OPT_RS_1H:	mhw.set(M_RS_1H);

	nCase OPT_VSYNC:	mhw.set(M_VSYNC);
	nCase OPT_SMPU_T:	mhw.set(M_SMPU_T);
	nCase OPT_FM:		mhw.set(M_FM);
	nCase OPT_MTP_T:	mhw.set(M_MTP_T);
	nCase OPT_TIMER:	mhw.set(M_TIMER);
	nCase OPT_MOUSE:	mhw.set(M_MOUSE);

	nCase OPT_TFREQ:		tFreq=gNum();
	nCase OPT_THRU:			gFlag(thru);
	nCase OPT_SYNCOUT:		gFlag(syncOut);
	nCase OPT_SENDFE:		gFlag(sendFe);
	nCase OPT_MIDISYNC:		gFlag(midiSync);
	nCase OPT_AUTOTEMPO:	gFlag(autoTempo);
	nCase OPT_MULTIPORT:	gFlag(multiPort);

	nCase OPT_COLOR:		Gvram::plt4096();
	nCase OPT_NEGA:			Gvram::pltNega();
	nCase OPT_MONO:			Gvram::pltMono();
	nCase OPT_NOTE:			Gvram::pltNote();
	nCase OPT_RNOTE:		Gvram::pltRnote();
	nCase OPT_GNOTE:		Gvram::pltGnote();

	nCase OPT_NOA:			aFlag=false;
	nCase OPT_NOB:			bFlag=false;
	nCase OPT_NOC:			cFlag=false;
	}
}

static void execArgs()
{
	AppArg
		arg;

	playFile[0]=0;
	arg.exec();
}

static void mInit()
{
	sint
		i;

	mhw.setDefault(0x1);
	mhw.setDefault(0x6);

	if(multiPort)
		midi.init(mhw);
	else
		midi.init1(mhw);
	midi.setOwnId("SPEANAKUN");

	if(midi.ports()==0){
		error=true;
		rcode=105;
		cout << "Error: " << "MIDIインターフェースが使えない" << endl;
		return;
	}

	if((midi.xclass() & 0x6)==0){
		error=true;
		rcode=106;
		cout << "Error: " << "時間管理ができない" << endl;
		return;
	}

	midi.mhw(mhw);

	midi.tFreq(tFreq);
	for(i=(multiPort?midi.ports()-1:0);i>=0;--i){
		midi.thru(i,thru);				// MIDIスルー
		midi.midiSync(i,midiSync);		// 外部同期
		midi.autoTempo(i,autoTempo);	// テンポ自動変更
		midi.activeSence(i,sendFe);		// アクティブセンシング出力
		midi.midiClockOut(i,syncOut);	// MIDIタイミングクロック出力
	}
}

static void mEnd()
{
	midi.end();
}

#define INITTYPE 2
#include "initinst.cpp"

static void gInit()
{
	gv.init1();
	pSave.store();				// パレットをセーブし、
	gSave.storeAll();			// 画面をセーブし、

	tv.fkeyOff();				// ファンクションガイドを消して
	tv.cls();					// テキスト画面を消して

	gv.minFade();				// フェードアウトして

	tt.pushMode();				// 30行状態をプッシュ
	tt.changeNormal();			// 25行モードに戻す

	gv.init2();
	gv.cls();

	midi.setMsPerVs();			// VSYNC周期が変わる(かもしれない)ので。
}

static void gEnd()
{
	gv.setBgColor(7,7,13);
	gv.minFade(16);
	gv.cls();
	gv.minFade(0,0,0);

	gv.end1();

	tt.popMode();

	gSave.restore();
	pSave.fadeRestore();

	gv.end2();
	tv.fkeyOn();
}

static void drawScreen()
{
	sint
		i,
		j,
		sy,
		t;

	drawBackground();						// 背景

	level.init();

	if(sType==stSMF)
		t=sSong.nTracks();
	else if(sType==stRCP)
		t=rSong.nTracks();
	else
		t=0;

	if(t<=12)
		sy=338;
	else if(t<=24)
		sy=322;
	else
		sy=306;

	for(i=0;i<3;++i){						// 顔
		for(j=0;j<12;++j){
			if(i*12+j<t){
				face[i*12+j].live();
				face[i*12+j].pos(10+j*25,sy+32*i);
				face[i*12+j].draw();
			}else{
				face[i*12+j].dead();
			}
		}
	}

	lcd.init();								// 液晶
	lcd.pos(320,325);
	lcd.draw();

	letter.init();							// 液晶文字
	letter.pos(320,305);
	letter.draw();

	spa.init();								// スペアナＡ
	spa.pos(10,181);
	if(aFlag)
		spa.draw();

	spb.init();								// 同Ｂ
	spb.pos(10,45);
	if(bFlag)
		spb.draw();

	spc.init();								// 同Ｃ
	spc.pos(467,164);
	spc.pos2(545,80);

	if(sType==stSMF){
		if(sSong.timeBase()>0)
			spc.timeBase(sSong.timeBase());
		else
			spc.timeBase(sSong.frameRate()*sSong.frameBit());
		if(cFlag)
			spc.draw();

		uchar
			tt[65];
		sSong.title(tt,sizeof tt);				// 曲名
		Ghar::BSpsXy(max(230-4*sint(strlen(tt)),0),10,tt,14,8);
	}else if(sType==stRCP){
		spc.timeBase(rSong.timeBase());
		if(cFlag)
			spc.draw();

		uchar
			tt[RcpHeader::TITLELEN+1];
		rSong.title(tt);							// 曲名
		for(i=RcpHeader::TITLELEN-1;i>=0;--i)
			if(tt[i]==' ')
				tt[i]='\0';
			else
				break;
		while(tt[0]==' ')
			strmove(tt,tt+1);
		Ghar::BSpsXy(max(230-4*sint(strlen(tt)),0),10,tt,14,8);
	}

	gv.maxFade();
}

static void updateScreen()
{
	sint
		i;

	for(i=0;i<FACES;++i)
		face[i].update();

	lcd.update();

	letter.update();

	if(aFlag)
		spa.update();

	if(bFlag)
		spb.update();

	if(cFlag)
		spc.update();
}

void child()
{
	_ExtMemSave
		es;
	PaletteSave
		pSave;

	gSave.storeAll();
	pSave.store();

	tv.fkeyOn();

	es.save();
	mySystem();
	es.restore();

	tv.fkeyOff();
	tv.cls();

	gv.aPage(0);
	pSave.restore();
	gSave.restore();
	gv.vPage(0);
	gv.on();
}

static void speanaMain()
{
	Keyboard
		kbd;
	KeyData
		key;
	sint
		vtimer=midi.vsTimer();

	initInst();

	if(sType==stSMF)
		sSong.playStart();
	else if(sType==stRCP)
		rSong.playStart();
	tifc.set(ti);
	vtifc.set(vti);

	while(1){
		updateScreen();

		if(	(sType==stSMF && !sSong.playing()) ||
			(sType==stRCP && !rSong.playing()) ){
			face[0].allSleep();		// 演奏終了したら顔全員寝ろ
			if(uTimeDiff(midi.vsTimer(),vtimer)>100)
				break;				// 演奏終了後VSYNC100個待ち終了
		}else{
			vtimer=midi.vsTimer();
		}

		if(kbd.get(key).isOKey()){
			if(key.isSPACE()){
				rcode=1;
				break;
			}
			if(key.isRET()){
				rcode=2;
				break;
			}
			if(key.isESC()){
				rcode=3;
				break;
			}
			if(key.isGRPH() && key.isBKSLASH()){
				child();
			}
		}
	}

	if(sType==stSMF)
		sSong.playStop();
	else if(sType==stRCP)
		rSong.playStop();
	tifc.reset();
	vtifc.reset();

	muteInst();
}

void fileTypeCheck()
{
	sType=stNONE;

	FileFind
		ff;

	compensatePath(playFile,"MIDPATH",".MID.RCP.R36");
	compensatePath(playFile,"RCPPATH",".RCP.R36");
	compensatePath(playFile,"MIDIPATH",".MID");

	if(ff.findFirst(playFile))
		strcpy(playFile,ff.name());

	iFileStrm
		is(playFile);

	if(!is.opened()){
		error=true;
		rcode=103;
		cout << "Error: " << playFile << ": ファイルがオープンできない"
				<< endl;
	}else if(sSong.isSmf(is)){
		sType=stSMF;
	}else if(rSong.isRcp(is)){
		sType=stRCP;
	}else{
		error=true;
		rcode=104;
		cout << "Error: " << playFile << ": 未対応データ" << endl;
	}
}

#include "..\eyecatch.cpp"

static void spectrumAnalyzer()
{
	gInit();

	eyecatch(7,7,13);

	sSong.outsight();
	rSong.outsight();

	if(sType==stSMF){
		sSong.load(playFile);
		sSong.insight();

		if(multiPort){
			for(sint i=0;i<SmfPlay::MAXPORTTAB;++i)
				sSong.setPortTab(i,i);
		}else{
			sSong.setPortTab(0);
		}
	}else if(sType==stRCP){
		rSong.load(playFile);
		rSong.insight();

		if(multiPort){
			for(sint i=0;i<(RcpPlay::MAXCHANNEL+15)/16;++i)
				rSong.setPort(i,i);
		}else{
			rSong.setPort(0);
		}
	}

	drawScreen();

	speanaMain();

	gEnd();
}

sint cdecl main()
{
	if(!Machine::is98()){
		cerr << PGM_ERR REQ_PC98 << endl;
		return 200;
	}
	if(Machine::is98hi()){
		cerr << PGM_ERR EXC_PC98HI << endl;
		return 201;
	}
	if(!OperatingSystem::versionIsLater(3,1)){
		cerr << PGM_ERR REQ_DOS31L << endl;
		return 202;
	}
	if(OperatingSystem::isWindows95()){
		cerr << PGM_ERR EXC_WINDOWS95 << endl;
		return 203;
	}
	if(OperatingSystem::isWindows()){
		cerr << PGM_ERR EXC_WINDOWS << endl;
		return 204;
	}
	if(Machine::is256color()){
		cerr << PGM_ERR EXC_GRAPH256 << endl;
		return 205;
	}

	ThisProgram::killStop();
	_OvrInitEms(0,0,0);
	_ExtMem::errFunc(poolErr);
	ThisProgram::exeName(myName);
	ThisProgram::freeEnvSeg();
	myRandomize();

	execArgs();

	if(*playFile=='\0'){
		cout << "スペアナくん１号 v" VERSTR "+" << midi.myVerStr() <<
				" " CREDIT1994 << endl;
		rcode=102;
	}else if(error==false){
		mInit();
		fileTypeCheck();
		if(error==false)
			spectrumAnalyzer();
		mEnd();
	}

	return rcode;
}


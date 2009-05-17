/*
		スペアナくん１号

		By Ken/ichiro(OPA)
*/

#ifndef SPEANA_defined
#define SPEANA_defined

#include "midi.h"
#include "midifunc.h"
#include "grph.h"
#include "rcpplay.h"
#include "smfplay.h"

#define FACES		36

void drawBackground();
void ti(void*);
void vti(void*);

class Level {						// Mastervol/Vol/Exp/Velを掛け算するクラス

public:
	enum { PARTS = 16 };

private:
	uchar
		_mvol,			// 0-127
		_vol[PARTS],	// 0-127
		_exp[PARTS];	// 0-127
	sint
		_ttl[PARTS];	// 0-127	Mastervol*Vol*Expを予め入れとくキャッシュ

	void setTtl(sint part);

public:
	void init();

	void mvol(uchar mv);
	void vol(sint part,uchar v);
	void exp(sint part,uchar e);

	sint comp(sint part,uchar vel);	// out:0-127

};

class Face {								// 顔です。画面を見ればわかる。

private:
	enum {
			LIVE,
			SLEEP,
			DEAD
		}_state;
	sint
		_x,
		_y,
		_eyeBlink,
		_eyePos,
		_mouth,
		_noteCount;
	uchar
		_eyeFlag,
		_mouthFlag;

	void drawEye();
	void drawPupil(sint t,sint b);
	void drawMouth();

public:
	void init()								{ dead(); }

	void live();
	void dead();
	void pos(sint x,sint y);

	void sleep();
	void allSleep();

	void draw();
	void update();

	void pan(sint pan);
	void on(sint lv);
	void off();

	void vti(sint vtimer);

};

class Lcd {									// 液晶です。見ればわかる

private:
	sint
		_x,
		_y,
		_pict;
	uint
		_dat[16];		// 絵の表示時UINT_BIT == 16 に依存
	uchar
		_mv[16];

	void draw(sint x);

public:
	void init();
	void pos(sint x,sint y);

	void draw();
	void update();

	void on(sint part,sint lv);
	void grph(cuchar *gdat);

	void vti(sint);

};

class Letter {								// 液晶の文字。見ればわかる

private:
	sint
		_x,
		_y,
		_letl,
		_lett;
	uchar
		_buf[16+32],
		_flag;

	void clear();

public:
	void init();
	void pos(sint x,sint y);

	void draw();
	void update();

	void set(cuchar *s,sint len);

	void vti(sint vtimer);

};

class Spa {									// 週刊誌ではない。スペアナＡ

private:
	enum {
			BARS = 44,
			HEIGHT = 216,
			RHEIGHT = 108,
			RBARW = 10
		};
	sint
		_x,
		_y,
		_bar[BARS],
		_ph[BARS],
		_pht[BARS];
	uchar
		_bm[BARS];

	void drawBar(sint b);
	void _on(sint b,sint lv,sint hg);

public:
	void init();
	void pos(sint x,sint y);

	void draw();
	void update();

	void on(sint key,sint lv);

	void vti(sint vtimer);

};

class Spb {								// PCM音源ボードではない。スペアナＢ

private:
	enum {
			PARTS = 16,
			WIDTH = 439,
			RBARH = 7
		};
	sint
		_x,
		_y,
		_bar[PARTS],
		_pan[PARTS],
		__pan[PARTS];
	uchar
		_bm[PARTS];

	void drawBar(sint b);

public:
	void init();
	void pos(sint x,sint y);

	void draw();
	void update();

	void on(sint part,sint lv);
	void pan(sint part,sint pan);

	void vti(sint vtimer);

};

class Spc {									// 空白ではない。スペアナＣ

public:
	enum {
			BARS = 32,
			HEIGHT = 360,
			WIDTH = 160,
			RADIUS = 65
		};

private:
	sint
		_x,
		_y,
		_x2,
		_y2,
		_tm,
		_tb,
		_sweep,
		_ph,
		__ph,
		_bar[BARS],
		_ox[BARS],
		_oy[BARS];
	uchar
		_bm[BARS];

	void reset();
	void drawBar(sint b);
	void drawCirc(sint r);
	void _on(sint b,sint hg);

public:
	void init();
	void pos(sint x,sint y);
	void pos2(sint x,sint y);
	void timeBase(sint tb);

	void draw();
	void update();

	void on(sint lv);
	void countUp(sint dt);
	void beat(sint nn,sint dd);

	void ti(sint timer);

};

class SSong : public SmfPlay {			// スペアナくん専用SMF演奏クラス
	typedef SmfPlay SuperClass;

protected:

	virtual void endOfTrack(sint tr);
	virtual void noteOff(sint,uchar ch,uchar key,uchar vel);
	virtual void noteOff(sint,uchar ch,uchar key);
	virtual void noteOn(sint,uchar ch,uchar key,uchar vel);
	virtual void ctrl(sint,uchar ch,uchar ccno,uchar val);
	virtual void sysex(sint,cuchar *d,sint len);

};

class RSong : public RcpPlay {			// スペアナくん専用RCP演奏クラス
	typedef RcpPlay SuperClass;

protected:

	virtual void endOfTrack(sint tr);
	virtual void noteOff(sint,uchar ch,uchar key);
	virtual void noteOn(sint,uchar ch,uchar key,uchar vel);
	virtual void ctrl(sint,uchar ch,uchar ccno,uchar val);
	virtual void sysex(sint,cuchar *d,sint len);

};

#ifdef	GVAR_DEF		// こういう情けない定義なんとかならんもんでしょうか。
#define Extern
#define _A(n)	=n
#else
#define Extern	extern
#define _A(n)
#endif

Extern Gvram
	gv;
Extern Tvram
	tv;
Extern Midi
	midi;
Extern Level
	level;
Extern Face
	face[FACES];
Extern Lcd
	lcd;
Extern Letter
	letter;
Extern Spa
	spa;
Extern Spb
	spb;
Extern Spc
	spc;
Extern TimerFunc
	tifc;
Extern VsTimerFunc
	vtifc;
Extern SSong
	sSong;
Extern RSong
	rSong;
Extern enum {stNONE,stSMF,stRCP}
	sType;

Extern bool
	aFlag		_A(true),
	bFlag		_A(true),
	cFlag		_A(true);

#undef Extern
#undef _A

#endif


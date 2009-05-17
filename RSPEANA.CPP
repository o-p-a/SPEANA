/*
		スペアナくん１号

		オーバーレイしてはいけない部分。

		By Ken/ichiro(OPA)
*/

#include "stdincl.h"
#pragma hdrstop
#include "opa.h"
#include "opalib.h"
#include "speana.h"

static uchar
	attTab[256]={
0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6
	};
		// ↑減衰量テーブル。こいつを取り替えればバーの減衰のパターンを
		// 変えられます。ちなみにこのパターンを出力するawkスクリプトは
/*	att.awk
BEGIN{

	printf("0");

	for(i=1;i<256;++i){
		v=i/50+1;

		printf(",%d",v+0.5);
	}
}
*/
		// ↑です。テーブルにする必要は全然ないのだった(苦笑)

static uchar
	spTab[35]={
4,6,9,14,21,29,40,54,70,89,109,131,153,173,191,204,213,216,213,204,191,173,153,131,109,89,70,54,40,29,21,14,9,6,4
	};
		// 正規分布テーブル。スクリプトは以下の通り。
		// sigmaがちょっと小さかったような気もするが直すのが面倒だ(笑)
/* sp.awk
BEGIN{
	PI=3.1415926535;

	mu=17;
	sigma=6;

	for(x=0;x<35;++x){
		n=exp(-((x-mu)^2)/(2*sigma^2));

		n*=216;

		if(x!=0)
			printf(",");
		printf("%d",n+0.5);
	}
}
*/
		// 参考文献：「統計入門」，和田秀三，サイエンス社

///////////////////////////////////////////////////////////////////////////////

void Level::setTtl(sint part)
{
	_ttl[part]=mltdiv45(mltdiv45(_mvol,_vol[part],127),_exp[part],127);
}

void Level::mvol(uchar mv)
{
	_mvol=mv;

	for(sint i=0;i<PARTS;++i)
		setTtl(i);
}

void Level::vol(sint part,uchar v)
{
	_vol[part]=v;

	setTtl(part);
}

void Level::exp(sint part,uchar e)
{
	_exp[part]=e;

	setTtl(part);
}

sint Level::comp(sint part,uchar vel)		// compareではないcompute。
{
	sint
		w=mltdiv45(_ttl[part],vel,127);
	minMaxLimit(w,0,127);
	return w;
}

///////////////////////////////////////////////////////////////////////////////

void Face::sleep()							// 顔は寝ました！
{
	if(_state==LIVE){
		_state=SLEEP;
		_eyeFlag=true;
	}
}

void Face::pan(sint pan)					// 目玉の向きを変えます！
{
	sint
		p=(pan+16)/32;

	if(p!=_eyePos){
		_eyePos=p;
		_eyeFlag=true;
	}
}

void Face::on(sint lv)						// 口をあけます！
{
	++_noteCount;

	sint
		m=lv/15;		// 0-8

	if(m>_mouth){
		_mouth=m;

		_mouthFlag=true;
	}
}

void Face::off()							// 口を閉じます！ あとで
{
	if(_noteCount>0)
		--_noteCount;
}

void Face::vti(sint vtimer)
{
	if(_state!=DEAD){
		if(_state==LIVE && (vtimer&1)==0){
			if(random(1500)==0){			// 瞬きします！
				_eyeBlink=5;
				_eyeFlag=true;
			}else if(_eyeBlink>0){			// ↑の続き
				--_eyeBlink;
				_eyeFlag=true;
			}
		}

		if(_noteCount>0){					// 口を閉じます！
			if(_mouth>4 && (vtimer&1)==0){
				--_mouth;
				_mouthFlag=true;
			}
		}else{
			if(_mouth>0){
				_mouth-=2;
				minLimit(_mouth,0);
				_mouthFlag=true;
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

void Lcd::on(sint part,sint lv)
{
	if(_pict<=0){
		if(_dat[part]<lv){
			_dat[part]=lv;
			_mv[part]=true;
		}
	}
}

void Lcd::grph(cuchar *gdat)				// 絵を描きます！
{
	uchar
		maskb;
	uint
		maskw,
		work;
	sint
		y,
		x;

	maskb=0x10;
	for(x=0;x<16;++x){
		work=0;
		maskw=0x8000U;
		for(y=0;y<16;++y){
			if(gdat[y] & maskb)
				work |= maskw;
			maskw>>=1;
		}
		if(_dat[x]!=work || _pict==0){
			_dat[x]=work;
			_mv[x]=true;
		}
		maskb>>=1;
		if(maskb==0){
			gdat+=16;
			maskb=0x10;
		}
	}

	_pict=150;
}

void Lcd::vti(sint)					// レベルメータの減衰／絵のタイムアウト
{
	if(_pict<=0){
		for(sint i=0;i<16;++i){
			if(_dat[i]>1){
				_dat[i]-=attTab[_dat[i]];
				_mv[i]=true;
			}
		}
	}else{
		if(--_pict<=0){
			for(sint i=0;i<16;++i){
				_dat[i]=0;
				_mv[i]=true;
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

void Letter::clear()
{
	_letl=0;
	_lett=0;
	memset(_buf,' ',sizeof _buf);
}

void Letter::set(cuchar *s,sint len)		// LCDに文字をセット
{
	clear();

	while(*s==' ' && len>0){
		++s;
		--len;
	}
	while(len>0 && s[len-1]==' ')
		--len;

	maxLimit(len,32);

	if(len>16){
		memcpy(_buf+15,s,len);
		_letl=len+15;
		_lett=17;
	}else{
		memcpy(_buf+(16+1-len)/2,s,len);
		_letl=0;
		_lett=150;
	}

	for(sint i=0;i<sizeof _buf;++i){
		if(_buf[i]==0)
			_buf[i]=' ';
	}

	_flag=true;
}

void Letter::vti(sint)				// LCD文字をスクロール／タイムアウトで消去
{
	if(_lett>0){
		if(_letl<=0){
			if(--_lett<=0){
				clear();
				_flag=true;
			}
		}else{
			if(--_lett<=0){
				memmove(_buf,_buf+1,sizeof _buf-1);
				if(--_letl>0)
					_lett=17;
				_flag=true;
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

void Spa::_on(sint b,sint lv,sint height)	// バー１本に新しい高さをセット
{
	sint
		h=mltdiv45(height,lv,127);

	if(b>=0 && b<BARS){
		if(_bar[b]<h){
			_bar[b]=h;
			_bm[b]=true;
		}
		if(_ph[b]<h){
			_ph[b]=h;
			_pht[b]=45;
			_bm[b]=true;
		}else if(_ph[b]==h){
			_pht[b]=45;
		}
	}
}

void Spa::on(sint key,sint lv)				// ノートオンの処理
{
	sint
		b=key/3 - 5,
		t=2-key%3;

	for(;t<sizeof spTab;t+=3,++b)
		_on(b,lv,spTab[t]);
}

void Spa::vti(sint /*vtimer*/)
{
	for(sint b=0;b<BARS;++b){
		if(_bar[b]>0){
			_bar[b]-=attTab[_bar[b]];
			_bm[b]=true;
		}
		if(_ph[b]>0){
			if(_pht[b]>0){
				_pht[b]--;
			}else{
				_ph[b]--;
				_bm[b]=true;
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

void Spb::on(sint part,sint lv)
{
	sint
		h=mltdiv45((WIDTH-1)/2,lv,127);

	if(_bar[part]<h){
		_bar[part]=h;
		_bm[part]=true;
	}
}

void Spb::pan(sint part,sint pan)
{
	if(_pan[part]!=pan){
		__pan[part]=_pan[part];
		_pan[part]=pan;
		_bm[part]=true;
	}
}

void Spb::vti(sint /*vtimer*/)
{
	for(sint b=0;b<PARTS;++b){
		if(_bar[b]>0){
			_bar[b]-=attTab[_bar[b]];
			_bm[b]=true;
		}
		if(_pan[b]!=__pan[b]){
			if(__pan[b]<_pan[b])
				__pan[b]+=(_pan[b]-__pan[b]+9)/10;
			else
				__pan[b]-=(__pan[b]-_pan[b]+9)/10;
			_bm[b]=true;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

void Spc::_on(sint b,sint hg)
{
	if(_bar[b]!=hg){
		_bar[b]=hg;
		_bm[b]=true;
	}
}

void Spc::on(sint lv)
{
	minLimit(__ph,mltdiv45(HEIGHT/2,lv,127));
}

void Spc::reset()
{
	_tm=0;
}

void Spc::countUp(sint dt)
{
	_tm+=dt;
}

void Spc::beat(sint,sint)
{
	reset();
}

void Spc::ti(sint)
{
	sint
		sw;

	while(_tm>=_tb)
		_tm-=_tb;
	if((sw=mltdiv45(_tm,BARS,_tb))>=BARS)
		sw=0;

	while(sw!=_sweep){
		if((_sweep & 1)==0)
			for(sint b=0;b<BARS;++b)
				if(_bar[b]>0){
					_bar[b]-=(_bar[b]+10)/11;
					_bm[b]=true;
				}

		_ph+=(__ph-_ph)/2+4;
		maxLimit(_ph,HEIGHT/2);
		if(_ph>=__ph){
			_ph=__ph;
			__ph=_ph*3/4;
		}
		_on(_sweep,_ph);

		if(++_sweep>=BARS)
			_sweep=0;
	}
}

///////////////////////////////////////////////////////////////////////////////

void _noteOn(sint tr,uchar ch,uchar key,uchar vel)
{
	if(ch<16){
		sint
			lv=level.comp(ch,vel);

		if(tr<FACES)
			face[tr].on(lv);

		lcd.on(ch,lv);

		spa.on(key,lv);

		spb.on(ch,lv);

		spc.on(lv);
	}else{
		if(tr<FACES)
			face[tr].on(vel);
	}
}

void _ctrl(sint tr,uchar ch,uchar ccno,uchar val)
{
	if(ch>=16)
		return;

	switch(ccno){
	case 0x7:			// Volume
		level.vol(ch,val);
	nCase 0xa:			// Panpot
		if(tr<FACES)
			face[tr].pan(val);
		spb.pan(ch,val);
	nCase 0xb:			// Expression
		level.exp(ch,val);
	};
}

void _sysex(cuchar *d,sint len)
{
	uchar
		sum=0;
	for(sint i=4;i<len;++i)
		sum+=d[i];

	if((sum & 0x7f)!=0)
		return;

	if(d[0]==0x41 && d[3]==0x12){		// Roland , DT1
		if(d[2]==0x42){					// GS
			if(d[4]==0x40){
				if(d[5]==0){
					if(d[6]==4){		// Master volume
						level.mvol(d[7]);
					}
				}
			}
		}else if(d[2]==0x45){			// SC55
			if(d[4]==0x10){
				if(d[5]==0x01){
					if(d[6]==0){		// LCD display
						lcd.grph(d+7);
					}
				}else if(d[5]==0){
					if(d[6]==0){		// LCD letter
						letter.set(d+7,len-8);
					}
				}
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

void SSong::endOfTrack(sint tr)
{
	SuperClass::endOfTrack(tr);

	if(tr<FACES)							// 寝ます！
		face[tr].sleep();
}

void SSong::noteOff(sint tr,uchar ch,uchar key,uchar vel)
{
	SuperClass::noteOff(tr,ch,key,vel);

	if(tr<FACES)
		face[tr].off();
}

void SSong::noteOff(sint tr,uchar ch,uchar key)
{
	SuperClass::noteOff(tr,ch,key);

	if(tr<FACES)
		face[tr].off();
}

void SSong::noteOn(sint tr,uchar ch,uchar key,uchar vel)
{
	SuperClass::noteOn(tr,ch,key,vel);

	_noteOn(tr,port(tr)*16+ch,key,vel);
}

void SSong::ctrl(sint tr,uchar ch,uchar ccno,uchar val)
{
	SuperClass::ctrl(tr,ch,ccno,val);

	_ctrl(tr,port(tr)*16+ch,ccno,val);
}

void SSong::sysex(sint tr,cuchar *d,sint len)
{
	SuperClass::sysex(tr,d,len);

	if(port(tr)==0)
		_sysex(d,len);
}

///////////////////////////////////////////////////////////////////////////////

void RSong::endOfTrack(sint tr)
{
	SuperClass::endOfTrack(tr);

	if(tr<FACES)							// 寝ます！
		face[tr].sleep();
}

void RSong::noteOff(sint tr,uchar ch,uchar key)
{
	SuperClass::noteOff(tr,ch,key);

	if(tr<FACES)
		face[tr].off();
}

void RSong::noteOn(sint tr,uchar ch,uchar key,uchar vel)
{
	SuperClass::noteOn(tr,ch,key,vel);

	_noteOn(tr,ch,key,vel);
}

void RSong::ctrl(sint tr,uchar ch,uchar ccno,uchar val)
{
	SuperClass::ctrl(tr,ch,ccno,val);

	_ctrl(tr,ch,ccno,val);
}

void RSong::sysex(sint tr,cuchar *d,sint len)
{
	SuperClass::sysex(tr,d,len);

	if(port((pi(tr).ch)>>4)==0)
		_sysex(d,len);
}

///////////////////////////////////////////////////////////////////////////////

void ti(void*)							// いろんなブツの時間を進めます
{
	static sint
		t;
	sint
		dt=sTimeDiff(midi.timer(),t);

	if(cFlag){
		spc.countUp(dt);
		spc.ti(midi.timer());
	}
	t+=dt;

	tifc.dt(1);
}

void vti(void*)
{
	sint
		vtimer=midi.vsTimer();

	for(sint i=0;i<FACES;++i)
		face[i].vti(vtimer);

	lcd.vti(vtimer);

	letter.vti(vtimer);

	if(aFlag)
		spa.vti(vtimer);

	if(bFlag)
		spb.vti(vtimer);

	vtifc.dt(1);
}


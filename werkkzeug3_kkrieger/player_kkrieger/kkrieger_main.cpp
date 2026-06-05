// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "_types.hpp"
#include "_start.hpp"
#include "_viruz2.hpp"
#include "_ogg.hpp"
#include "kdoc.hpp"
#include "kkriegergame.hpp"
#include "engine.hpp"
#include "genoverlay.hpp"
#include "rtmanager.hpp"
#include "kkrieger_enhanced.hpp"
#include <stdio.h>

/****************************************************************************/

static CV2MPlayer *Sound;
static sMusicPlayer *Player = 0;
static sInt SoundTimer;
KDoc *Document;
KEnvironment *Environment;
KKriegerGame *Game;
sF32 GlobalFps;
sInt DebugTexMem = 0;

extern "C" sU8 DebugData[];
static sU8* PtrTable[] =
{
  (sU8 *) 0x54525450,     // entry 0: export data ('PTRT')
  (sU8 *) 0x454C4241,     // entry 1: viruzII tune ('ABLE')
};

extern sInt IntroLoop;

/****************************************************************************/

/****************************************************************************/

static void KKRLog(const sChar *text)
{
#if KKR_PLAYER_TRACE_LOG
  FILE *file = fopen("player_kkrieger.log","ab");
  if(file)
  {
    fputs(text,file);
    fputs("\r\n",file);
    fclose(file);
  }
#endif
}

static const sU8 *KKRLoadPlayerData()
{
  const sChar *paths[] =
  {
    sSystem->GetCmdLine(),
    "data\\kkrieger3383.kx",
    "..\\data\\kkrieger3383.kx",
    "..\\..\\data\\kkrieger3383.kx",
    "werkkzeug3_kkrieger\\data\\kkrieger3383.kx",
    "data\\kkrieger.k",
    "werkkzeug3_kkrieger\\data\\kkrieger.k",
  };

  for(sInt i=0;i<sizeof(paths)/sizeof(paths[0]);i++)
  {
    if(paths[i] && paths[i][0])
    {
      KKRLog(paths[i]);
      sU8 *data = sSystem->LoadFile(paths[i]);
      if(data)
      {
        KKRLog("loaded data");
        return data;
      }
    }
  }

  return 0;
}

static void KKRFitAspectViewport(sViewport &vp,sF32 targetAspect)
{
  sInt sx = sSystem->ConfigX;
  sInt sy = sSystem->ConfigY;
  sInt x0,y0,x1,y1;

  if(sx<=0 || sy<=0 || targetAspect<=0)
  {
    vp.Window.Init(0,0,sx,sy);
    return;
  }

  if((sF32)sx/sy > targetAspect)
  {
    sInt width = sInt(sy*targetAspect + 0.5f);
    x0 = (sx-width)/2;
    y0 = 0;
    x1 = x0+width;
    y1 = sy;
  }
  else
  {
    sInt height = sInt(sx/targetAspect + 0.5f);
    x0 = 0;
    y0 = (sy-height)/2;
    x1 = sx;
    y1 = y0+height;
  }

  vp.Window.Init(x0,y0,x1,y1);
}

static sF32 KKRInitGameViewport(sViewport &vp)
{
  vp.Init();

#if KKR_ENHANCED_BUILD
  switch(KKR_ASPECT_MODE)
  {
  case KKR_ASPECT_FULLSCREEN_16X9:
    KKRFitAspectViewport(vp,16.0f/9.0f);
    return 1.0f*vp.Window.XSize()/vp.Window.YSize();
  case KKR_ASPECT_FULLSCREEN_16X10:
    KKRFitAspectViewport(vp,16.0f/10.0f);
    return 1.0f*vp.Window.XSize()/vp.Window.YSize();
  case KKR_ASPECT_FULLSCREEN_21X9:
    KKRFitAspectViewport(vp,21.0f/9.0f);
    return 1.0f*vp.Window.XSize()/vp.Window.YSize();
  case KKR_ASPECT_STRETCH_DEBUG:
    vp.Window.Init(0,0,sSystem->ConfigX,sSystem->ConfigY);
    return 2.0f;
  case KKR_ASPECT_ORIGINAL:
  default:
    break;
  }
#endif

  vp.Window.Init(0,sSystem->ConfigY*1/6,sSystem->ConfigX,sSystem->ConfigY*5/6);
  return 1.0f*vp.Window.XSize()/vp.Window.YSize();
}

struct VFXEntry
{
  sU8 MajorId;
  sU8 MinorId;
  sS8 Gain;
  sU8 reserved;
  sU32 StartPos;
  sU32 EndPos;
  sU32 LoopSize;
};

static void RenderSoundEffects(KDoc *doc,sU8 *data)
{
  const sInt maxsamples = 0x10000;
  static sF32 smpf[maxsamples*2];
  static sS16 *smps = (sS16 *) smpf;
  sU32 *data32,size;
  sU8 *v2m;
  sInt i,j,count,start,len,fade;
  sF32 amp,ampx;
  VFXEntry *ent;

  sVERIFY(Sound);

  data32 = (sU32 *) data;
  sVERIFY(data32[0] == sMAKE4('V','F','X','0'));
  size = data32[1];
  v2m = (sU8 *) &data32[2];
  data32 = (sU32 *) (((sU8 *)data32)+size+12);
  count = *data32++;
  ent = (VFXEntry *) data32;

  if(Sound->Open(v2m))
  {
    for(i=0;i<count;i++)
    {
      start = ent->StartPos;
      if(ent->LoopSize)
        start = ent->EndPos - ent->LoopSize;
      len = sMin<sInt>(ent->EndPos - start,maxsamples);
      ampx = sFPow(10.0f,(ent->Gain / 5.0f) / 20.0f);

      Sound->Play(ent->StartPos);
      Sound->Render(smpf,len);
      fade = 1000;
      if(ent->MajorId & 1)
        fade = 44100;

      for(j=0;j<len;j++)
      {
        amp = ampx;
        if(j<fade)
          amp = amp*j/fade;
        if(j>len-fade)
          amp = amp*(len-j)/fade;

        smps[j*2+0] = sRange<sInt>(smpf[j*2+0]*amp*0x7fff,0x7fff,-0x7fff);
        smps[j*2+1] = sRange<sInt>(smpf[j*2+1]*amp*0x7fff,0x7fff,-0x7fff);
      }

      sSystem->SampleAdd(smps,len,4,ent->MinorId,!(ent->MajorId & 1));
      sSystem->Progress(doc->Ops.Count+i,doc->Ops.Count+count);
      ent++;
    }

    Sound->Close();
  }
}

/****************************************************************************/

void IntroSoundHandler(sS16 *stream,sInt left,void *user)
{
  static sF32 buffer[4096*2];
  sF32 *fp;
  sInt count;
  sInt i;

  SoundTimer += left;
  if(Sound)
  {
    while(left>0)
    {
      count = sMin<sInt>(4096,left);
      Sound->Render(buffer,count);
      fp = buffer;
      for(i=0;i<count*2;i++)
        //*stream++ = 0;
        *stream++ = sRange<sInt>(*fp++*0x7fff,0x7fff,-0x7fff);
      left-=count;
    }
#pragma lekktor(off)
    if(SoundTimer>=(2*60+25)*44100)
    {
      Sound->Open(Document->SongData);
      Sound->Play(0);
      SoundTimer = 0;
    }
#pragma lekktor(on)
  }
  else
  {
    for(i=0;i<left*2;i++)
      *stream++ = 0;
    SoundTimer = 0;
  }
}

static void MusicPlayerHandler(sS16 *buffer,sInt samples,void *user)
{
  sMusicPlayer *player = (sMusicPlayer *) user;
  player->Render(buffer,samples);
}

extern sBool ConfigDialog(sInt nr);

sBool sAppHandler(sInt code,sDInt value)
{
  sInt beat;
  const sU8 *data;
  sViewport vp;
  sInt i,max;
  sF32 gameZoomY;
  sF32 curfps;
  static sF32 oldfps;
  static sInt FirstTime,ThisTime,LastTime,sample;
  static sInt framectr=0;
  
  KOp *root;

  //GenBitmapTextureSizeOffset = -1;
  switch(code)
  {
#if !sINTRO
  case sAPPCODE_CONFIG:
#if KKR_ENHANCED_BUILD
    KKRLog("config: windowed 1280x720");
    sSetConfig(sSF_DIRECT3D,1280,720);
#else
    sSetConfig(sSF_DIRECT3D|sSF_FULLSCREEN,800,600);
#endif
    break;
#endif

  case sAPPCODE_INIT:
    KKRLog("init: begin");
    data = PtrTable[0];
    if(((sInt)data)==0x54525450)
    {
      KKRLog("init: loading external data");
      data = KKRLoadPlayerData();
      if(data==0)
      {
        KKRLog("init: data load failed");
        sSystem->Abort("need data file: pass one on the command line or run near data\\kkrieger.k");
      }
    }
    KKRLog("init: KDoc new");
    Document = new KDoc;
    KKRLog("init: KDoc Init");
    Document->Init(data);
    KKRLog("init: Environment new");
    Environment = new KEnvironment;
    Sound = 0;

    KKRLog("init: perlin");
    sInitPerlin();   
    KKRLog("init: overlay");
    GenOverlayInit();

    KKRLog("init: render targets");
    RenderTargetManager = new RenderTargetManager_;

    KKRLog("init: engine");
    Engine = new Engine_;

    KKRLog("init: game");
    Game = new KKriegerGame;
    Game->Init();

    KKRLog("init: env setup");
    sFloatFix();
    i = sSystem->GetTime();
    Environment->Splines = &Document->Splines.Array;
    Environment->SplineCount = Document->Splines.Count;
    Environment->Game = Game;
    Environment->InitView();
    Environment->InitFrame(0,0);
    KKRLog("init: precalc begin");
    Document->Precalc(Environment);
    KKRLog("init: precalc end");
    KKRLog("paint: exit frame");
    Environment->ExitFrame();

    if(Document->SongSize)
    {
      KKRLog("init: sound begin");
      Sound = new CV2MPlayer;

      if(Document->SampleSize)
      {
        KKRLog("init: sound effects begin");
        RenderSoundEffects(Document,Document->SampleData);
        KKRLog("init: sound effects end");
      }

      if(Sound->Open(Document->SongData))
      {
        KKRLog("init: music open ok");
        Sound->Play(0);
        SoundTimer = 0;
#if KKR_ENABLE_AUDIO
        sSystem->SetSoundHandler(IntroSoundHandler,64);
#else
        KKRLog("init: sound handler skipped");
#endif
      }
      else
      {
        KKRLog("init: music open failed");
        delete Sound;
        Sound = 0;
      }
      KKRLog("init: sound end");
    }

    KKRLog("init: reset root");
    Game->ResetRoot(Environment,Document->RootOps[Document->CurrentRoot],1);

    FirstTime = sSystem->GetTime();
    LastTime = 0;
    oldfps = 0.0f;
    KKRLog("init: end");
    break;
#if !sINTRO || sPROJECT == sPROJ_SNOWBLIND
  case sAPPCODE_EXIT:
    KKRLog("exit: begin");
    sSystem->SetSoundHandler(0,0,0);
    if(Sound)
    {
      CV2MPlayer *old;
      old = Sound;
      Sound = 0;
      delete old;
    }
    delete Player;
    delete Environment;
    Game->Exit();
    delete Game;
    Document->Exit();
    delete Document;
    delete Engine;
    delete RenderTargetManager;
    GenOverlayExit();
    KKRLog("exit: end");
    break;
#endif
  case sAPPCODE_PAINT:
    KKRLog("paint: begin");
    // tick processing (moved up to reduce input lag by 1 frame)

    ThisTime = sSystem->GetTime() - FirstTime;

    beat = sMulDiv(ThisTime,Document->SongBPM,60000);

    curfps = ThisTime - LastTime;
    oldfps += (curfps - oldfps) * 0.1f;
    GlobalFps = 1000.0f / oldfps;

    max = ThisTime/10-LastTime/10;
    if(max>10) max = 10;

    sFloatFix();
    KKRLog("paint: tick");
    Game->OnTick(Environment,max);
    sSystem->Sample3DCommit();
    LastTime = ThisTime;

    sFloatFix();

    // root-switching logic and outermost stuff

    KKRLog("paint: viewport");
    gameZoomY = KKRInitGameViewport(vp);
    GenOverlayManager->SetMasterViewport(vp);

    sInt mode;
    mode = Game->GetNewRoot();
    KKRLog("paint: root mode");

    if(mode!=Document->CurrentRoot)
    {
      sSystem->SetSoundHandler(0,0);
      Document->CurrentRoot = mode;
      Environment->InitView();
      Environment->InitFrame(0,0);
      Document->Precalc(Environment);
      KKRLog("paint: exit frame");
      Environment->ExitFrame();
      Game->ResetRoot(Environment,Document->RootOps[Document->CurrentRoot],0);

      Sound->Open(Document->SongData);
      Sound->Play(0);
#if KKR_ENABLE_AUDIO
      sSystem->SetSoundHandler(IntroSoundHandler,64);
#else
      KKRLog("init: sound handler skipped");
#endif
    }

    // timing

    KKRLog("paint: frame events");
    Environment->InitFrame(beat,ThisTime);
    Document->AddEvents(Environment);
    Game->AddEvents(Environment);
    Game->FlushPhysic();

    // game painting

    KKRLog("paint: clear");
    sSystem->Clear(sVCF_ALL,0);

    KKRLog("paint: camera");
    Environment->GameCam.Init();
    Game->GetCamera(Environment->GameCam);
    Environment->GameCam.ZoomY = gameZoomY;

    root = Document->RootOps[Document->CurrentRoot];

    KKRLog("paint: root exec check");
    if(root->Cache->ClassId==KC_DEMO)
    {
      GenOverlayManager->Reset(Environment);
      GenOverlayManager->RealPaint = sTRUE;
      GenOverlayManager->Game = Game;

      sFloatFix();
      KKRLog("paint: root exec");
      root->Exec(Environment);
      KKRLog("paint: root exec done");

      GenOverlayManager->RealPaint = sFALSE;
      GenOverlayManager->Reset(Environment);
    }

    sFloatFix();
    KKRLog("paint: exit frame");
    Environment->ExitFrame();
    Environment->Mem.Flush();
//    sSystem->SetWinMouse(vp.Window.x1/2,vp.Window.y1/2);
    break;

  case sAPPCODE_KEY:
#if sPROJECT==sPROJ_KKRIEGER
    Game->OnKey(value);
    if(value == (sKEY_ESCAPE|sKEYQ_SHIFT) || value==sKEY_CLOSE)
      sSystem->Exit();
#else
    if(value == sKEY_ESCAPE)
      sSystem->Exit();
#endif
    break;
#if !sINTRO
  default:
    return sFALSE;
#endif
  }

  return sTRUE;
}

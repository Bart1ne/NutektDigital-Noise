#define IS_B 0
#include "unit_genericfx.h"
#include <stdint.h>
#include <math.h>
namespace {
constexpr float TWOPI=6.28318530718f;
static float sr=48000.f, low=0.f, band=0.f, brown=0.f, pink0=0.f, pink1=0.f, lfo=0.f, env=0.f;
static uint32_t rng=0x91AC3E7Du;
static int32_t params[8]={0,512,120,300,0,300,512,1};
static bool touching=false;
static float touchGain=0.f;
static float lastPulse=0.f, filterF=0.12f;
static unsigned filterCounter=0;
static float clampf(float v,float a,float b){return v<a?a:(v>b?b:v);}
static float randb(){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return ((rng>>8)*(1.f/8388608.f))-1.f;}
static void resetDSP(){touchGain=0.f;low=band=brown=pink0=pink1=lfo=env=lastPulse=0.f;filterCounter=0;filterF=0.12f;}
static float processSample(){
 float noise=randb();
 pink0=0.99765f*pink0+0.099046f*noise;
 pink1=0.96300f*pink1+0.296516f*noise;
 float pink=clampf((pink0+pink1+0.1848f*noise)*0.58f,-1.f,1.f);
 brown=clampf(0.997f*brown+0.029f*noise,-1.f,1.f);
 float rate=0.2f+float(params[5])*0.025f;
 lfo+=rate/sr; if(lfo>=1.f)lfo-=1.f;
 float motion=float(params[4])/1023.f;
 float mod=IS_B ? sinf(lfo*TWOPI)*motion : 0.f;
 float tone=clampf(float(params[1])/1023.f+0.28f*mod,0.f,1.f);
 if((filterCounter++ & 31u)==0u){
   const float cutoff=80.f*powf(200.f,tone);
   filterF=2.f*sinf(3.14159265f*clampf(cutoff,30.f,15000.f)/sr);
 }
 const float f=filterF;
 const float q=clampf(1.8f-1.72f*float(params[2])/1023.f,0.08f,1.8f);
 const float density=float(params[6])/1023.f;
 lastPulse+=(1.f+65.f*density)/sr;
 if(lastPulse>=1.f){lastPulse-=1.f;env=1.f;}
 env*=0.985f-0.009f*density;
 const int type=params[0];
 float src=noise;
 if(type==1)src=pink;
 else if(type==2)src=brown*2.f;
 else if(type==3)src=noise*env*2.f;
#if IS_B
 else if(type==4) {
   const float probability=0.00002f+0.008f*density;
   src=(randb()>1.f-2.f*probability)?randb()*3.f:0.f;
 } else if(type==5)src=(pink*0.5f+noise*0.4f+brown*0.5f+noise*env);
#else
 else if(type==4)src=(pink*0.5f+noise*0.5f+brown*0.45f);
#endif
 float y=0.f;
 for(int k=0;k<2;k++){
   const float h=f*0.5f;
   low+=h*band;
   const float high=src-low-q*band;
   band+=h*high;
   y=low;
 }
 float amplitude=0.36f*float(params[3])/1023.f;
#if IS_B
 amplitude*=1.f-0.55f*motion*(0.5f-0.5f*sinf(lfo*TWOPI));
#endif
 float v=clampf(y*amplitude,-0.9f,0.9f);
 // Touch gate: attack ~3ms, release ~7ms, always silent at startup.
 const float target=(params[7]==0 || touching)?1.f:0.f;
 const float coefficient=(target>touchGain)?(1.f/(0.003f*sr)):(1.f/(0.007f*sr));
 touchGain+=coefficient*(target-touchGain);
 if(touchGain<0.00001f && target==0.f)touchGain=0.f;
 v*=touchGain;
 return v/(1.f+fabsf(v));
}
}
extern "C" {
__unit_callback int8_t unit_init(const unit_runtime_desc_t *d){
 if(!d)return k_unit_err_undef;
 if(d->target!=unit_header.common.target)return k_unit_err_target;
 if(!UNIT_API_IS_COMPAT(d->api))return k_unit_err_api_version;
 if(d->samplerate!=48000U)return k_unit_err_samplerate;
 if(d->input_channels!=2 || d->output_channels!=2)return k_unit_err_geometry;
 sr=(float)d->samplerate;
 for(unsigned i=0;i<8;i++)params[i]=unit_header.common.params[i].init;
 resetDSP(); touching=false; return k_unit_err_none;
}
__unit_callback void unit_teardown(void){}
__unit_callback void unit_reset(void){resetDSP();}
__unit_callback void unit_resume(void){}
__unit_callback void unit_suspend(void){}
__unit_callback void unit_render(const float *in,float *out,uint32_t frames){
 (void)in;
 for(uint32_t i=0;i<frames;i++){
  float y=processSample();out[2*i]=y;out[2*i+1]=y;
 }
}
__unit_callback void unit_set_param_value(uint8_t id,int32_t v){
 if(id>=8)return;
 if(v<unit_header.common.params[id].min)v=unit_header.common.params[id].min;
 if(v>unit_header.common.params[id].max)v=unit_header.common.params[id].max;
 params[id]=v;
}
__unit_callback int32_t unit_get_param_value(uint8_t id){return id<8?params[id]:0;}
__unit_callback const char *unit_get_param_str_value(uint8_t id,int32_t v){
 if(id==7)return v?"TOUCH":"FREE";
 if(id==0){
#if IS_B
 static const char *names[]={"WHITE","PINK","BROWN","PULSE","CRACKLE","MIX"};
 return (v>=0 && v<6)?names[v]:"WHITE";
#else
 static const char *names[]={"WHITE","PINK","BROWN","PULSE","MIX"};
 return (v>=0 && v<5)?names[v]:"WHITE";
#endif
 }
 return "";
}
__unit_callback void unit_touch_event(uint8_t id,uint8_t phase,uint32_t x,uint32_t y){
 (void)id;(void)x;(void)y;
 if(phase==k_unit_touch_phase_began || phase==k_unit_touch_phase_moved || phase==k_unit_touch_phase_stationary)touching=true;
 if(phase==k_unit_touch_phase_ended || phase==k_unit_touch_phase_cancelled)touching=false;
}
__unit_callback void unit_set_tempo(uint32_t tempo){(void)tempo;}
__unit_callback void unit_tempo_4ppqn_tick(uint32_t counter){(void)counter;}
}

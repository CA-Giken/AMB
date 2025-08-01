#include "Logger.h"
#ifdef ARDUINO
#include "Arduino.h"
#else
#include <string.h>
#include <math.h>
uint32_t micros(){
  auto alog=logger::trace(logger::length-1);
  return alog->stamp;
}
#endif

#ifndef NULL
#define NULL 0
#endif


#define BUFMIN 200   //minimum window size

namespace logger{
#ifdef TARGET_NRF52840
  static struct ALOG buf[1440];
  void ALOG::print(){
#ifdef ARDUINO
    Serial.print(stamp); Serial.print(" ");
    Serial.print(duty); Serial.print(" ");
    Serial.print(beta); Serial.print(" ");
    Serial.print(eval); Serial.print(" ");
    Serial.print(interval); Serial.print(" ");
    Serial.print(mode); Serial.print(" ");
    Serial.print(latency); Serial.print(" ");
    Serial.print(omega); Serial.print(" ");
    Serial.println(cmd);
#endif
  }
#endif
#ifdef _RENESAS_RA_
#define RINGBUF
  static struct ALOG buf[BUFMIN];
  void ALOG::print(){
#ifdef ARDUINO
    Serial.print(stamp); Serial.print(" ");
    Serial.print(duty); Serial.print(" ");
    Serial.print(beta); Serial.print(" ");
    Serial.println(eval);
#endif
  }
#endif
  static struct ALOG *data=buf;
  struct ALOG stage;
  static int32_t tzero;
  int16_t length;
  static int16_t nsweep;
  void clear(){
    memset(&stage,0,sizeof(stage));
    stage.stamp=0xFFFFFFFF;
  }
  inline int limit(){
    return sizeof(buf)/sizeof(buf[0]);
  }
  void start(){
    clear();
    tzero=micros();
    length=nsweep=0;
  }
  void latch(){
    if(stage.stamp==0xFFFFFFFF) stage.stamp=micros()-tzero;
    buf[length%limit()]=stage;
    length++;
    stage.stamp=0xFFFFFFFF;
  }
  ALOG *trace(int index){
    if(index>=length) return NULL;
    else if(index>(int)length-limit()) return buf+(index%limit());
    else return NULL;
  }
  template <typename buffer_t> float variation(buffer_t *dat,int samp){
    float sig=0;
    for(int i=0;i<samp;i++){
      int d=dat[i];
      sig+=d*d;
    }
    return sqrt(sig/samp);
  }
  float N(int n1,int dim,int wgh){
    int16_t cval[BUFMIN];
    int nsamp=length-n1;
    if(nsamp>BUFMIN) nsamp=BUFMIN;
    for(int i=0;i<nsamp;i++){
      cval[i]=logger::trace(n1+i)->beta;
    }
    double coef[POLYNOMINAL];
    int nofs=approx<int16_t>(dim,cval,nsamp,wgh,coef);
    auto neq=[&](double x){
      double y=0;
      x=(x-nofs)/nofs;
      for(int i=dim-1;i>=0;i--) y=y*x+coef[i];
      return y;
    };
    for(int i=0;i<nsamp;i++){
      int cc=neq(i);
      int cv=cval[i];
      cval[i]=(cv-cc);
#ifdef ONE_SPAN_TEST
      printf("%lu %d %d %d %d %d %d\n",pv[i].stamp,pv[i].beta,pv[i].sigma,pv[i].duty,cv,cc,cval[i]);
#endif
    }
    float sig=variation<int16_t>(cval,nsamp);
    return sig;
  }
  float analyze(int samp,int dim,int wgh){
    if(samp<1000)   //"samp" defined as data count, else as interval in micro second
      return N(length-samp,dim,wgh);

    int n1=length-1;
    logger::ALOG *p=logger::trace(n1);
    int ts2=p->stamp;
    for(;;n1--){
      p=logger::trace(n1);
      if(p==NULL){
        n1++;
        break;
      }
      if(ts2 - p->stamp > samp) break;
    }
    return N(n1,dim,wgh);
  }
  void sweep(){
#ifdef ARDUINO
    if(nsweep>=length) return;
    ALOG *p=trace(nsweep++);
    Serial.print("$$ ");
    Serial.print(nsweep);
    if(p!=NULL){
      Serial.print(" ");
      p->print();
    }
    else Serial.println();
#endif
  }
}

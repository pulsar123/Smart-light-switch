// Modified Sunrise library. Original was taken here: https://github.com/chaeplin/Sunrise

#ifndef Sunrise2_h
#define Sunrise2_h

class Sunrise2{
  public:
  Sunrise2(float, float, float);
  void Custom(float);
  void Actual();
  void Civil();
  void Nautical();
  void Astronomical();
  int Rise(unsigned char ,unsigned char );
  int Set(unsigned char ,unsigned char );
  int Noon(unsigned char ,unsigned char );
  unsigned char Hour();
  unsigned char Minute();
  
  private:
  int Compute(unsigned char ,unsigned char, int);
  float lat,lon, zenith, rd, tz;
  unsigned char  theHour,theMinute;
};

#endif

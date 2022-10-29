// PT100test
// Tools > Board > DOIT ESP32 DEVKIT V1

// +------------+    180 Ω
// |         3V3|---[SHUNT]---+
// |            |             |
// | ESP32 GPIO4|-------------+
// |            |             |
// |         GND|---[PT100]---+
// +------------+
const int   adcpin = 4;


// Physics based mapping of ADC value to temperature in °C
int physics(int adc) {
  const float R_shunt = 182.0; // actual (measured)
  static float average = -1;
  if( average == -1 ) average = adc; else average = average*0.9 + 0.1*adc;
  float V_pt100 = 3.3*adc/4095.0 + 0.1855; // + 0.1372; // https://w4krl.com/esp32-analog-to-digital-conversion-accuracy/
  float V_shunt = 3.3 - V_pt100;
  float I_shunt = V_shunt/R_shunt;
  float R_pt100 = V_pt100/I_shunt;
  float T_pt100 = (R_pt100-100)/0.385055;
  //Serial.printf("adc:% 4d\tmu=%4.0f\tVp:%5.3f\tRp:%3d\tT:%d\n",adc,average,V_pt100,(int)R_pt100,(int)T_pt100);
  return (int)T_pt100;
}


// Hand callibrated lookup table mapping ADC value to temperature in °C
int lookup(int adc ) {
  #define NUM 10
  const int adcv[NUM] = {1285,1680,1724,1769,1814,1850,1885,1916,1942,1972};
  const int tmpv[NUM] = {   0, 160, 180, 200, 220, 240, 260, 280, 300, 320};

  // Binary search to find rank (rank is number of entries in the lookup table that are strictly smaller)
  //  adc:1284  rank:0
  //  adc:1285  rank:0
  //  adc:1286  rank:1
  //  adc:1971  rank:9
  //  adc:1972  rank:9
  //  adc:1973  rank:10
  int left = 0;
  int right = NUM;
  while( left<right ) {
    int mid = (left+right)/2;
    if( adcv[mid]<adc ) left=mid+1; else right=mid;
  }
  int rank = left;
  
  // Linear interpolation using two table entries
  int tmp;
  if( rank==NUM ) tmp = map(adc,adcv[rank-2],adcv[rank-1],tmpv[rank-2],tmpv[rank-1]);
  else if( rank==0  ) tmp = map(adc, adcv[rank],adcv[rank+1],tmpv[rank],tmpv[rank+1]);
  else tmp = map(adc, adcv[rank-1],adcv[rank],tmpv[rank-1],tmpv[rank]);

  //Serial.printf("adc:%d\trank:%d\tT:%d\n",adc,rank,tmp);
  return tmp;
}


void setup() {
  Serial.begin(115200); delay(100);
  Serial.printf("\n\nPT100test\n");
}


void loop() {
  int sum_tmp = 0;
  int64_t sum_tmp2 = 0;
  int num = 0;
  int overflow = 0;
  uint32_t timems = millis();
  while( millis()-timems<5000 ) {
    int adc = analogRead(adcpin);
    int tmp = lookup(adc); // physics(adc);
    int64_t s2 = sum_tmp2 + tmp*tmp;
    overflow = s2<sum_tmp2;
    if( overflow ) break;
    sum_tmp = sum_tmp + tmp;
    sum_tmp2 = s2;
    num += 1;
  }
  timems = millis() - timems;
  float mu = sum_tmp / num;
  float var = sum_tmp2 / num - mu*mu;
  float sd = sqrt(var);
  Serial.print("n="); Serial.print(num); Serial.print(overflow?"v":""); Serial.print(" ");
  Serial.print("t="); Serial.print(timems); Serial.print("ms ");
  //Serial.print("sx="); Serial.print(sum_tmp); Serial.print(" ");
  //Serial.print("sx2="); Serial.print(sum_tmp2); Serial.print(" ");
  Serial.print("mu="); Serial.print((int)(mu+0.5)); Serial.print("°C ");
  //Serial.print("var="); Serial.print(var); Serial.print(" ");
  Serial.print("sd=");Serial.println(sd);
}

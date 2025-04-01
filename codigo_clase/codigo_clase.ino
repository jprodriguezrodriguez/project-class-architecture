#include <EEPROM.h>

#include <LiquidCrystal.h>
LiquidCrystal lcd(13, 12, 11, 10, 9, 8);
#include <Key.h>
#include <Keypad.h>
#include <Wire.h>  //memoria externa
#define DS1307_I2C_ADDRESS 0X68

int clave[4], i, b, dato1, dato2;
byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
char datom;
long segundos, auxsegundos;
int datoi[4], direc, direccion;
int dato = 0x00;
int pines[] = {2, 3, 4, 5}; 
byte estados[] = {HIGH, LOW};
int tiempo = 200;

const byte ROWS = 4;
const byte COLS = 4;
byte dir = 0x11;
short emptyPosition = false;
int date[7];
char keys[ROWS][COLS] = {
  { '1', '2', '3', 'A' },
  { '4', '5', '6', 'B' },
  { '7', '8', '9', 'c' },
  { '*', '0', '#', 'D' }
};
//                     F1 F2 F3 F4
byte rowPins[ROWS] = { 37, 35, 33, 31 };
//                    C1 C2 C3 C4
byte colPins[COLS] = { 45, 43, 41, 39 };
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

byte decToBcd(byte val) {
  return ((val / 10 * 16) + (val % 10));
}

byte bcdToDec(byte val) {
  return ((val / 16) * 10 + (val % 16));
}

/** Funcion para Leer el RTC ***/
void getDateDs1307(
  byte *second,
  byte *minute,
  byte *hour,
  byte *dayOfWeek,
  byte *dayOfMonth,
  byte *month,
  byte *year
) {
  Wire.beginTransmission(DS1307_I2C_ADDRESS);
  Wire.write (0);
  Wire.endTransmission ();

  Wire.requestFrom(DS1307_I2C_ADDRESS, 7);

  *second = bcdToDec(Wire.read()&0x7f);
  *minute = bcdToDec(Wire.read());
  *hour = bcdToDec(Wire.read()&0x3f);
  *dayOfWeek = bcdToDec(Wire.read());
  *dayOfMonth = bcdToDec(Wire.read());
  *month = bcdToDec(Wire.read());
  *year = bcdToDec(Wire.read());
}
/** Funcion para Escribir en el RTC ***/
void setDateDs1307(byte second,      // 0-59
                   byte minute,      // 0-59
                   byte hour,        // 0-23
                   byte dayOfWeek,   // 1-7
                   byte dayOfMonth,  // 1-28/29/30/31
                   byte month,       // 1-12
                   byte year)        // 0-99
{
  Wire.beginTransmission(DS1307_I2C_ADDRESS);
  Wire.write(0);
  Wire.write(decToBcd(second));  // 0 to bit 7 starts the clock
  Wire.write(decToBcd(minute));
  Wire.write(decToBcd(hour));  // If you want 12 hour am/pm you need to set
  Wire.write(decToBcd(dayOfWeek));
  Wire.write(decToBcd(dayOfMonth));
  Wire.write(decToBcd(month));
  Wire.write(decToBcd(year));
  Wire.endTransmission();  
}

void i2c_eeprom_write_byte(int deviceaddress, unsigned int eeaddress, byte data) {
  int rdata=data;
  Wire.beginTransmission (deviceaddress);
  Wire.write ((int)(eeaddress>>8)); //MSB
  Wire.write ((int)(eeaddress & 0XFF)); //LSB
  Wire.write (rdata);
  Wire.endTransmission();
}

byte i2c_eeprom_read_byte(int deviceaddress, unsigned int eeaddress, unsigned int eeprom) {
  byte rdata = 0xFF;
  Wire.beginTransmission(deviceaddress);
  Wire.write((int)(eeaddress>>8));
  Wire.write((int)(eeaddress & 0xFF));
  Wire.endTransmission();
  Wire.requestFrom (deviceaddress,1);
  if(Wire.available()) {
    rdata = Wire.read();
  }
  return rdata;
}

void setup() {
  lcd.begin(16, 2);
  Serial.begin(9600);
  Wire.begin();
  inicializarMotor();
  apagarMotor();
}

void horaFecha() {
  // updateClock();
  getDateDs1307(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Date:");
  lcd.print(dayOfMonth, DEC);
  lcd.print("/");
  lcd.print(month, DEC);
  lcd.print("/");
  lcd.print(year, DEC);

  for (int i = 0; i < 10; i++) {
    getDateDs1307(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);
    lcd.setCursor(0, 1);
    lcd.print("Hour:");
    lcd.print(hour, DEC);
    lcd.print(":");
    lcd.print(minute, DEC);
    lcd.print(":");
    lcd.print(second, DEC);
    delay(990);
    lcd.setCursor(15, 0);
    lcd.print("D");
    lcd.setCursor(15, 1);
    lcd.print(dayOfWeek, DEC);
  }
}

void updateClock() {
  dayOfMonth = 0x19;
  month = 0x03;
  year = 0x19;
  dayOfWeek = 0x02;
  hour = 0x13;
  minute = 0x22;
  second = 0x00;

  setDateDs1307(second, minute,
    hour,
    dayOfWeek,
    dayOfMonth,
    month,
    year);
}

// Leer posición de memoria donde deso almacenar el dato
// ¿La posición en memoria está vacía (contiene FF)? guardarAqui : moverOchoPosiciones -> Vuelve al inicio
// Se genera un reporte cada vez que el ususario seleccione la opción "REPORTE" en el menú
//i2c_eeprom_read_byte(0x50, dir);  // Buscar este método jaja
void getEmptyMemoryPosition() {
  do {
    dato = 0x00;
    if (dato == 0xFF) {
      emptyPosition = true;
    } else {
      dir = dir + 8;
    }                                             // 248
  } while (emptyPosition = false && dir < 0xF8);  // última posición en memoria disponible para el primer usuario
}

void buildReport() {
  getDateDs1307(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);
  date[0] = dayOfMonth;
  date[1] = month;
  date[2] = year;
  date[3] = hour;
  date[4] = minute;
  date[5] = second;
  date[6] = 0x0C;  // ID DEL USUARIO
}

void voltearDerecha() {
  digitalWrite(pines[0], LOW);
  digitalWrite(pines[1], HIGH);
  digitalWrite(pines[2], HIGH);
  digitalWrite(pines[3], LOW);
  delay(tiempo);

  digitalWrite(pines[0], HIGH);
  digitalWrite(pines[1], LOW);
  digitalWrite(pines[2], LOW);
  digitalWrite(pines[3], HIGH);
  delay(tiempo);
}

void apagarMotor() {
  digitalWrite(pines[0], LOW);
  digitalWrite(pines[1], LOW);
  digitalWrite(pines[2], LOW);
  digitalWrite(pines[3], LOW);
}

void inicializarMotor() {
  pinMode(pines[0], OUTPUT);
  pinMode(pines[1], OUTPUT);
  pinMode(pines[2], OUTPUT);
  pinMode(pines[3], OUTPUT);
}

void loadMotor(){
  voltearDerecha();
  delay(2000);
  apagarMotor();
  delay(3000);
}

void saveReport() {
  getEmptyMemoryPosition();
  if (emptyPosition) {
    buildReport();
    for (i = 0; i < 7; i++) {
      //i2c_eeprom_read_byte(0x50, dir, date[i]);
      dir++;
      delay(5);
    }
  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("MEMORIA LLENA");
  }
}

void teclado() {
  lcd.clear();  // limpiar la pantalla
  auxsegundos = 0;
  segundos = 0;
  b = 6;
  for (i = 0; i < 4; i++) {
    do {
      datom = keypad.getKey();
      delay(60);
      if (datom != '\0')  // SI DATOM ES DIFERENTE DE NADA
      {
        switch (i + 1) {
          case 1:
            lcd.setCursor(6, 1);
            lcd.print(datom);
            clave[0] = datom - 0x30;
            clave[0] = (clave[0] << 4);  // igual que la instruccion swap
            lcd.setCursor(6, 0);
            lcd.print("*");
            delay(50);
            break;

          case 2:
            lcd.setCursor(7, 1);
            lcd.print(datom);

            clave[1] = datom - 0x30;
            clave[0] = clave[0] + clave[1];
            lcd.setCursor(7, 0);
            lcd.print("*");
            delay(50);
            break;

          case 3:
            lcd.setCursor(8, 1);
            lcd.print(datom);

            clave[2] = datom - 0x30;
            clave[2] = (clave[2] << 4);  // igual que la instruccion swap
            lcd.setCursor(8, 0);
            lcd.print("*");
            delay(50);
            break;

          case 4:
            lcd.setCursor(9, 1);
            lcd.print(datom);

            clave[3] = datom - 0x30;
            clave[1] = clave[2] + clave[3];
            lcd.setCursor(9, 0);
            lcd.print("*");
            delay(50);
            break;
        }
        i++;
        delay(500);
        auxsegundos = 0;
        segundos = 0;
      }
      auxsegundos++;
      if (auxsegundos > 20) {
        auxsegundos = 0;
        segundos++;
      }
    } while (i < 4 && segundos < 6);
    clave[i] = "#";
    lcd.setCursor(b, 0);
    lcd.print("*");
    auxsegundos = 0;
    segundos = 0;
    b++;
  }
}

void leer_memoria() {
  for (i = 0; i < 6; i++) {
    datoi[i] = EEPROM.read(i);
    delay(5);
  }
}

void escribir_memoria() {
  for (i = 0; i < 4; i++) {
    EEPROM.write(direc, clave[i]);
    delay(5);
    direc++;
  }
}

void unDigito() {
  short flag = false;
  do {
    datom = keypad.getKey();
    if (datom != '\0') {
      lcd.setCursor(8, 0);
      lcd.print(datom);
      delay(2000);
      datom = datom - 0x30;
      flag = true;
    }

  } while (flag == false);
}

void loop() {
inicio:
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("  BIENVENIDOS");
  lcd.setCursor(0, 1);
  lcd.print("ARQUITECTURA 401");
  delay(3000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("GRUPO 11: JENNY");
  delay(1000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("GRUPO 11: JOSE");
  delay(1000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("GRUPO 11: JUAN");
  delay(1000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("   POR FAVOR");
  lcd.setCursor(0, 1);
  lcd.print(" INGRESE CLAVE");
  delay(1000);
  leer_memoria();  // datoi[0] = 0xbb, datoi[1] = 0xbb, datoi[2] = 0xbb, datoi[3] = 0xbb,
  teclado();

  if (clave[0] == 0x23 & clave[1] == 0x45) {
menu:
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(" BIENVENIDOS");
    lcd.setCursor(0, 1);
    lcd.print(" MENU DISENADOR");
    delay(2000);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("1. PAOLA");
    lcd.setCursor(0, 1);
    lcd.print("2. JOSE");
    delay(2000);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("3. JUAN");
    delay(1000);
    lcd.clear();
    unDigito();
    if (datom == 0x01) {
      goto paola;
    }
    if (datom == 0x02) {
      goto jose;
    }
    if (datom == 0x03) {
      goto juan;
    }

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("ERROR DE NUMERO");
    delay(3000);
    goto menu;
  }
  if (bcdToDec(clave[0]) == bcdToDec(EEPROM.read(0)) && bcdToDec(clave[1]) == bcdToDec(EEPROM.read(1))) {
paola:
    direc = 0x00;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("BIENVENIDA JENNY");
    lcd.setCursor(0, 1);
    lcd.print(" DIGITE OPCION");
    delay(2000);
    lcd.clear();
    lcd.print("1.CAMBIO DE CLAVE");
    lcd.setCursor(0, 1);
    lcd.print("2.HORA Y FECHA");
    delay(3000);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("3.SERVOMOTORES");
    lcd.setCursor(0, 1);
    lcd.print("4.SALIR");
    delay(3000);
    lcd.clear();
    unDigito();
    if (datom == 0x01) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(" CAMBIO DE CLAVE");
      delay(3000);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("   INGRESE SU");
      lcd.setCursor(0, 1);
      lcd.print("  NUEVA CLAVE");
      delay(3000);
      teclado();
      dato1 = clave[0];
      dato2 = clave[1];
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("  CONFIRME SU");
      lcd.setCursor(0, 1);
      lcd.print("  NUEVA CLAVE");
      delay(3000);
      teclado();
      if (clave[0] == dato1 & clave[1] == dato2) {
        escribir_memoria();
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("    CLAVE");
        lcd.setCursor(0, 1);
        lcd.print("   ACTUALIZADA");
        delay(3000);
        goto paola;
      }
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ERROR DE CLAVE");
      delay(3000);
      goto paola;
    }
    if (datom == 0x02) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("HORA Y FECHA ");
      delay(2000);
      horaFecha();
      delay(5000);
      goto paola;
    }
    if (datom == 0x03) {
      loadMotor();
      goto paola;
    }
    if (datom == 0x04) {
      goto inicio;
    }
  }

  if (bcdToDec(clave[0]) == bcdToDec(EEPROM.read(2)) && bcdToDec(clave[1]) == bcdToDec(EEPROM.read(3))) {
jose:
    direc = 0x02;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("BIENVENIDO JOSE");
    lcd.setCursor(0, 1);
    lcd.print(" DIGITE OPCION");
    delay(2000);
    lcd.clear();
    lcd.print("1.CAMBIO DE CLAVE");
    lcd.setCursor(0, 1);
    lcd.print("2.HORA Y FECHA");
    delay(3000);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("3.SALIR");
    delay(2000);
    lcd.clear();
    unDigito();
    if (datom == 0x01) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(" CAMBIO DE CLAVE");
      delay(3000);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("   INGRESE SU");
      lcd.setCursor(0, 1);
      lcd.print("  NUEVA CLAVE");
      delay(3000);
      teclado();
      dato1 = clave[0];
      dato2 = clave[1];
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("  CONFIRME SU");
      lcd.setCursor(0, 1);
      lcd.print("  NUEVA CLAVE");
      delay(3000);
      teclado();
      if (clave[0] == dato1 & clave[1] == dato2) {
        escribir_memoria();
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("    CLAVE");
        lcd.setCursor(0, 1);
        lcd.print("   ACTUALIZADA");
        delay(3000);
        goto jose;
      }
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ERROR DE CLAVE");
      delay(3000);
      goto jose;
    }
     if (datom == 0x02) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("HORA Y FECHA ");
      delay(2000);
      horaFecha();
      delay(5000);
      goto jose;
    }
    if (datom == 0x03) {
      goto inicio;
    }
  }
  if (bcdToDec(clave[0]) == bcdToDec(EEPROM.read(4)) && bcdToDec(clave[1]) == bcdToDec(EEPROM.read(5))) {
juan:
    direc = 0x04;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("BIENVENIDO JUAN");
    lcd.setCursor(0, 1);
    lcd.print(" DIGITE OPCION");
    delay(2000);
    lcd.clear();
    lcd.print("1.CAMBIO DE CLAVE");
    lcd.setCursor(0, 1);
    lcd.print("2.HORA Y FECHA");
    delay(3000);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("3.SALIR");
    delay(2000);
    lcd.clear();
    unDigito();
    if (datom == 0x01) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(" CAMBIO DE CLAVE");
      delay(3000);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("   INGRESE SU");
      lcd.setCursor(0, 1);
      lcd.print("  NUEVA CLAVE");
      delay(3000);
      teclado();
      dato1 = clave[0];
      dato2 = clave[1];
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("  CONFIRME SU");
      lcd.setCursor(0, 1);
      lcd.print("  NUEVA CLAVE");
      delay(3000);
      teclado();
      if (clave[0] == dato1 & clave[1] == dato2) {
        escribir_memoria();
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("    CLAVE");
        lcd.setCursor(0, 1);
        lcd.print("   ACTUALIZADA");
        delay(3000);
        goto juan;
      }
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ERROR DE CLAVE");
      delay(3000);
      goto juan;
    }
     if (datom == 0x02) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("HORA Y FECHA ");
      delay(2000);
      horaFecha();
      delay(5000);
      goto juan;
    }
    if (datom == 0x03) {
      goto inicio;
    }
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("ERROR DE CLAVE");
  delay(3000);
  goto inicio;
}
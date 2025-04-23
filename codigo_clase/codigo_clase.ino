#include <EEPROM.h>

#include <LiquidCrystal.h>
LiquidCrystal lcd(13, 12, 11, 10, 9, 8);
#include <Key.h>
#include <Keypad.h>
#include <Wire.h>
#include <SD.h>
#include <SPI.h>
#define DS1307_I2C_ADDRESS 0X68
#define Q1 46
#define Q2 44
#define Q3 42
#define Q4 40
#define STQ 19

int clave[4], i, b, dato1, dato2;
byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
char datom;
long segundos, auxsegundos;
int datoi[4], direc, direccion;
int dato = 0x00;
int pines[] = { 2, 3, 4, 5 };
byte estados[] = { HIGH, LOW };
int tiempo = 2;
File myFile;
File reportFile;
int pinCS = 49;
byte userId = 0x0C;
int reportNumber = 0;

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
  byte *year) {
  Wire.beginTransmission(DS1307_I2C_ADDRESS);
  Wire.write(0);
  Wire.endTransmission();

  Wire.requestFrom(DS1307_I2C_ADDRESS, 7);

  *second = bcdToDec(Wire.read() & 0x7f);
  *minute = bcdToDec(Wire.read());
  *hour = bcdToDec(Wire.read() & 0x3f);
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
  int rdata = data;
  Wire.beginTransmission(deviceaddress);
  Wire.write((int)(eeaddress >> 8));    //MSB
  Wire.write((int)(eeaddress & 0XFF));  //LSB
  Wire.write(rdata);
  Wire.endTransmission();
}

byte i2c_eeprom_read_byte(int deviceaddress, unsigned int eeaddress) {
  byte rdata = 0xFF;
  Wire.beginTransmission(deviceaddress);
  Wire.write((int)(eeaddress >> 8));
  Wire.write((int)(eeaddress & 0xFF));
  Wire.endTransmission();
  Wire.requestFrom(deviceaddress, 1);
  if (Wire.available()) {
    rdata = Wire.read();
  }
  return rdata;
}

void setup() {
  lcd.begin(16, 2);
  Serial.begin(9600);
  Wire.begin();
  inicializarMotor();
  pinMode(pinCS, OUTPUT);
  Serial.println("pinCS");
  Serial.println(pinCS);
  Serial.println("\nInicializando tarjeta SD");
  do {
    if (!SD.begin(pinCS)) {
      Serial.println("Coloque una SD en el socket");
    }
  } while (!SD.begin(pinCS));

  Serial.println("SD conectada");

  myFile = SD.open("test.txt", FILE_WRITE);
  myFile.println("#;Reporte;Id equipo;Dia;Mes;Año");
  myFile.close();

  pinMode(STQ, INPUT);
  pinMode(Q1, INPUT);
  pinMode(Q2, INPUT);
  pinMode(Q3, INPUT);
  pinMode(Q4, INPUT);
}

void horaFecha() {
  updateClock();
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
  dayOfMonth = 0x16;
  month = 0x04;
  year = 0x19;
  dayOfWeek = 0x02;
  hour = 0x07;
  minute = 0x32;
  second = 0x00;

  setDateDs1307(second, minute, hour, dayOfWeek, dayOfMonth, month, year);
}

void voltearDerecha() {
  digitalWrite(pines[0], LOW);
  digitalWrite(pines[1], LOW);
  digitalWrite(pines[2], LOW);
  digitalWrite(pines[3], HIGH);
  delay(tiempo);
  digitalWrite(pines[0], LOW);
  digitalWrite(pines[1], LOW);
  digitalWrite(pines[2], HIGH);
  digitalWrite(pines[3], LOW);
  delay(tiempo);
  digitalWrite(pines[0], LOW);
  digitalWrite(pines[1], HIGH);
  digitalWrite(pines[2], LOW);
  digitalWrite(pines[3], LOW);
  delay(tiempo);
  digitalWrite(pines[0], HIGH);
  digitalWrite(pines[1], LOW);
  digitalWrite(pines[2], LOW);
  digitalWrite(pines[3], LOW);
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

void buildReport() {
  // updateClock();
  getDateDs1307(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);
  date[0] = dayOfMonth;
  date[1] = month;
  date[2] = year;
  date[3] = hour;
  date[4] = minute;
  date[5] = second;
}

void saveReportToSD(byte userId) {
  buildReport();
  uint8_t reportNum = getNextReportNumber(userId);

  char filename[20];
  sprintf(filename, "rep-%d.txt", userId);
  Serial.println(filename);
  reportFile = SD.open(filename, FILE_WRITE);
  if (reportFile) {
    reportFile.print("#");
    if (reportNumber < 10) reportFile.print("0");
    reportFile.print(reportNum);
    reportFile.print("\t");
    reportFile.print(date[0]);
    reportFile.print("\t");
    reportFile.print(date[1]);
    reportFile.print("\t");
    reportFile.print(date[2]);
    reportFile.print("\t");
    reportFile.print(date[3]);
    reportFile.print("\t");
    reportFile.print(date[4]);
    reportFile.print("\t");
    reportFile.print(date[5]);
    reportFile.print("\t");
    reportFile.print(userId);
    reportFile.println();
    reportFile.close();
    for (i = 0; i < 6; i++) {
      Serial.print("Guardando - ");
      Serial.print(i);
      Serial.print(" - ");
      Serial.print(date[i]);
      Serial.println();
    }
    Serial.println("Reporte guardado");
  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Error al guardar");
  }
}

void printReports(byte userId) {
  lcd.clear();
  char filename[20];
  sprintf(filename, "rep-%d.txt", userId);

  reportFile = SD.open(filename);
  if (reportFile) {
    while (reportFile.available()) {
      String linea = reportFile.readStringUntil('\n');
      char buffer[50];
      linea.toCharArray(buffer, sizeof(buffer));

      int campos[8];
      int index = 0;
      char *token = strtok(buffer, "\t");
      while (token != NULL && index < 8) {
        if (index == 0 && token[0] == '#') token++;
        campos[index++] = atoi(token);
        token = strtok(NULL, "\t");
      }

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("#");
      if (campos[0] < 10) lcd.print("0");
      lcd.print(campos[0]);
      lcd.print("  ");
      if (campos[1] < 10) lcd.print("0");
      lcd.print(campos[1]);
      lcd.print("/");
      if (campos[2] < 10) lcd.print("0");
      lcd.print(campos[2]);
      lcd.print("/");
      lcd.print(campos[3]);

      lcd.setCursor(0, 1);
      if (campos[4] < 10) lcd.print("0");
      lcd.print(campos[4]);
      lcd.print(":");
      if (campos[5] < 10) lcd.print("0");
      lcd.print(campos[5]);
      lcd.print(":");
      if (campos[6] < 10) lcd.print("0");
      lcd.print(campos[6]);
      lcd.print("  U:");
      lcd.print(campos[7]);
      delay(1500);
    }
    reportFile.close();
  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("No hay reportes");
  }
}

uint8_t getNextReportNumber(byte userId) {
  char filename[20];
  sprintf(filename, "rep-%d.txt", userId);
  File reportFile = SD.open(filename, FILE_READ);
  uint8_t count = 0;

  if (reportFile) {
    while (reportFile.available()) {
      String line = reportFile.readStringUntil('\n');
      if (line.length() > 0) {
        count++;
      }
    }
    reportFile.close();
  }
  return (count < 30) ? count + 1 : 0;
}

char decodificarDTMF() {
  uint8_t number_pressed;
  if (digitalRead(STQ) == LOW) {
    return 0;
  }

  delay(250);
  number_pressed = (digitalRead(Q1) << 0) | (digitalRead(Q2) << 1) | (digitalRead(Q3) << 2) | (digitalRead(Q4) << 3);

  switch (number_pressed) {
    case 0x01: Serial.println("DTMF: 1"); return '1';
    case 0x02: Serial.println("DTMF: 2"); return '2';
    case 0x03: Serial.println("DTMF: 3"); return '3';
    case 0x04: Serial.println("DTMF: 4"); return '4';
    case 0x05: Serial.println("DTMF: 5"); return '5';
    case 0x06: Serial.println("DTMF: 6"); return '6';
    case 0x07: Serial.println("DTMF: 7"); return '7';
    case 0x08: Serial.println("DTMF: 8"); return '8';
    case 0x09: Serial.println("DTMF: 9"); return '9';
    case 0x0A: Serial.println("DTMF: 0"); return '0';
    case 0x0B: Serial.println("DTMF: *"); return '*';
    case 0x0C: Serial.println("DTMF: #"); return '#';
    default:
      Serial.print("DTMF invalido: ");
      Serial.println(number_pressed);
      return 0;
  }
}


void teclado() {
  lcd.clear();
  auxsegundos = 0;
  segundos = 0;
  b = 6;
  for (i = 0; i < 4; i++) {
    do {
      datom = keypad.getKey();
      delay(60);
      if (datom != '\0') {
        switch (i + 1) {
          case 1:
            lcd.setCursor(6, 1);
            lcd.print(datom);
            clave[0] = datom - 0x30;
            clave[0] = (clave[0] << 4);
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
            clave[2] = (clave[2] << 4);
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
  leer_memoria();
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
    lcd.print("1.CAMBIAR CLAVE");
    lcd.setCursor(0, 1);
    lcd.print("2.HORA Y FECHA");
    delay(3000);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("3.SERVOMOTORES");
    lcd.setCursor(0, 1);
    lcd.print("4.REPORTES");
    delay(3000);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("5.DTMF 8870");
    lcd.setCursor(0, 1);
    lcd.print("6.SALIR");
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
      } else {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("ERROR DE CLAVE");
        delay(3000);
        goto paola;
      }
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
      for (i = 0; i < 3000; i++) {
        voltearDerecha();
        delay(tiempo);
      }
      goto paola;
    }
    if (datom == 0x04) {
      saveReportToSD(0x01);
      reportNumber++;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("GUARDANDO");
      lcd.setCursor(0, 1);
      lcd.print("REPORTE");
      delay(1000);
      lcd.clear();
      printReports(0x01);
      delay(3000);
      goto paola;
    }
    if (datom == 0x05) {
      lcd.clear();
      lcd.print("Modo DTMF Activo");
      char teclaDTMF = 0;

      // Bucle de espera hasta que se presione una tecla física
      do {
        if (digitalRead(STQ) == HIGH) {
          Serial.println("inside digital read");
          teclaDTMF = decodificarDTMF();
          if (teclaDTMF) {
            lcd.clear();
            lcd.print("DTMF: ");
            lcd.print(teclaDTMF);
            delay(1000);
            lcd.setCursor(0, 1);
            lcd.print("Esperando...");
          }
        }

        datom = keypad.getKey();  // Para salir del modo
      } while (datom == NO_KEY);

      lcd.clear();
      lcd.print("Saliendo...");
      delay(1000);
      lcd.clear();
      goto inicio;


      goto inicio;
    }
    if (datom == 0x06) {
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
      } else {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("ERROR DE CLAVE");
        delay(3000);
        goto jose;
      }
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
      for (i = 0; i < 5000; i++) {
        voltearDerecha();
        delay(tiempo);
      }
      goto jose;
    }
    if (datom == 0x04) {
      saveReportToSD(0x02);
      reportNumber++;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("GUARDANDO");
      lcd.setCursor(0, 1);
      lcd.print("REPORTE");
      delay(1000);
      lcd.clear();
      printReports(0x02);
      delay(3000);
      goto jose;
    }
    if (datom == 0x05) {
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
      } else {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("ERROR DE CLAVE");
        delay(3000);
        goto juan;
      }
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
      for (i = 0; i < 5000; i++) {
        voltearDerecha();
        delay(tiempo);
      }
      goto juan;
    }
    if (datom == 0x04) {
      saveReportToSD(0x03);
      reportNumber++;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("GUARDANDO");
      lcd.setCursor(0, 1);
      lcd.print("REPORTE");
      delay(1000);
      lcd.clear();
      printReports(0x03);
      delay(3000);
      goto juan;
    }
    if (datom == 0x05) {
      goto inicio;
    }
  } else {
    Serial.println("ELSE GLOBAL");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("ERROR DE CLAVE");
    delay(3000);
    goto inicio;
  }
}
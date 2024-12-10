#include <iostream>
#include <string>
#include <cstring>
#include <cmath>
#include <list>

#include <Arduino.h>
#include <Wire.h>
#include "SHT31.h"//lib capteur humidite
#include "ADS1X15.h" // lib adcs
#include <Wire.h>
#include "rgb_lcd.h"
#include <Servo.h>

#define DUREE_COMPTEUR 5000
#define RESOLVE_E_BP 44

//Instance des capteurs
SHT31 sht31 = SHT31(); //instance capteur humidite
ADS1115 ADS(0x48);  // ADS1115 physiquement défini à l'adresse 0x48, avec sa broche ADDR reliée à la masse
rgb_lcd lcd;
Servo myservo;

//config ecran couleur lcd
const int colorR = 255;
const int colorG = 0;
const int colorB = 0;

//varglobal
 bool flag_bouton =false;
 bool flag_touchb =false;
 bool flag_enigme_bp=false ;
 long lasttimecpt = 0;

  int pos = 0;

  bool enigme_hum=false ;
  bool enigme_light=false;
  bool enigme_bouton=false;
  bool enigme_potar=false;

//Declarations des classes
class enigme {
  protected :
    //LCD (Nbre d'enigme restante : n)
    static bool val_e_hum ;
    static bool val_e_light;
    static bool val_e_potar ;
    static bool val_e_bouton;
    static bool val_e_micro;  
    static int n;
    static int nb_enigme_old;
  public :
    void setNb_enigme(int nb){
      nb_enigme_old=nb;
    }   
    int getNb_enigme(){
      return nb_enigme_old;
    }
    virtual bool testEnigme()=0;
};

//initilisation des static de enigme
  bool enigme::val_e_hum=false ;
  bool enigme::val_e_light=false;
  bool enigme:: val_e_potar=false ;
  bool enigme::val_e_bouton=false;
  bool enigme::val_e_micro=false; 
  int enigme::nb_enigme_old=4;
  int enigme::n=4;
//

class e_hum : public enigme{
  private :
  float humidite;
  public:
  float getHum(){
    humidite = sht31.getHumidity();
    return humidite;
  }
  virtual bool testEnigme (){
    if (getHum() > 80){
      val_e_hum = true;
      //methode d'affichage
    }
    return val_e_hum;
  }
};

class e_light : public enigme{
  private :  
   float light;
  public:
  float getLight(){
    light= ADS.readADC(0);
    return light;
  }
  virtual bool testEnigme (){
    if (getLight() > 16000){
      val_e_light = true;
      //methode d'affichage
    }
    return val_e_light;
  }
};

class e_potar : public enigme{
  private :  
  public :
  virtual bool testEnigme(){
      if(pos >= 180){
        val_e_potar = true;
      }
    return val_e_potar;
  }
};

class e_bouton : public enigme{
  private :  
  int compteur;
  public :
  int getCompteur(){
    //compteur=digitalRead(D7)+compteur;
    return compteur;
  }
    void setCompteur(int cpt){
    //compteur=digitalRead(D7)+compteur;
     compteur= cpt;
  }
  int addCompteur(){
    //compteur=digitalRead(D7)+compteur;
    return compteur++;
  }
  virtual bool testEnigme (){
    if (compteur == RESOLVE_E_BP){
      val_e_bouton = true;
      //methode d'affichage
    }
    return val_e_bouton;
  }

 
};

class e_restart :  public enigme{
  private :
  //bool val_touch;
  public : 
    void restart(){
      val_e_hum=false ;
      val_e_light=false;
      val_e_potar=false ;
      val_e_bouton=false;  
      setNb_enigme(4);    
      pos=0;
    }
    virtual bool testEnigme (){
      val_e_hum=false ;
      val_e_light=false;
      val_e_potar=false ;
      val_e_bouton=false;
      return 0;
    }
};



  //instance class
 // enigme test;
  e_hum test_hum ;
  e_light test_light;
  e_bouton test_bouton;
  e_restart test_touch;
  e_potar test_potar;
 // int e_bouton::compteur=0;

//-----------Fonction d'interuption
 IRAM_ATTR void interuption_bouton(){
  flag_bouton = true; //leve le flag
  }
IRAM_ATTR void interuption_touchb(){
  flag_touchb = true;
  }
IRAM_ATTR void inter_enigme_bp(){
  flag_enigme_bp = true;
}
//-------------fonctions d'affichage-----------
void affichage_demarrage(){
    lcd.setRGB(colorR, colorG, colorB);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("BONJOUR");
    delay(2500);
    lcd.clear();
    lcd.print("Essayez d'ouvir");
    lcd.setCursor(0, 1);
    lcd.print("la boite");
    delay(2500);
    lcd.clear();
    lcd.print("Bonne chance");
    delay(2500);
}
void affichage_normal(int nb_e){
    //lcd.clear();
    lcd.setCursor(0, 0);
    lcd.printf("Reste %d Enigme",nb_e);
}
void affichage_final(){
  lcd.clear();
  lcd.setRGB(0, 255, 0);
  lcd.setCursor(0 ,0);
  lcd.printf("Bravo, vous avez");
  lcd.setCursor(0 ,1);
  lcd.printf("ouvert la boite");
}


//-----------------------Fonction Setup-------------------
void setup() {
  // put your setup code here, to run once:
  //pinMode(A0,INPUT);  //light sensor
  pinMode(D1,INPUT);
  pinMode(D7,INPUT); //bouton poussoir
  pinMode(D6,INPUT); //touch bouton
  pinMode(D4,OUTPUT);//servo
  pinMode(D5,OUTPUT);//buzz
  //------------Configuration du capteur humidite(SHT31)
  sht31.begin();  
  Serial.begin(9600);
  //------------Configuration du lcd
  lcd.begin(16, 2);
  lcd.setRGB(colorR, colorG, colorB);
  //--------------Configuration du servomoteur
  myservo.attach(D4);
  myservo.write(pos);
   //------------ADS1115(ADC)
  Wire.begin();
  ADS.begin();         // Initialisation du module ADS1115
  ADS.setGain(0);      // On prend le gain le plus bas (index 0), pour avoir la plus grande plage de mesure (6.144 volt)
  ADS.setMode(1);      // On indique à l'ADC qu'on fera des mesures à la demande, et non en continu (0 = CONTINUOUS, 1 = SINGLE)
  ADS.setDataRate(7);  // On spécifie la vitesse de mesure de tension qu'on souhaite, allant de 0 à 7 (7 étant le plus rapide, soit 860 échantillons par seconde)
  ADS.readADC(0);      // Et on fait une lecture à vide, pour envoyer tous ces paramètres
  //---------configuration des interruptions
  attachInterrupt(digitalPinToInterrupt(D7), interuption_bouton, FALLING);
  attachInterrupt(digitalPinToInterrupt(D6), interuption_touchb, RISING);
  attachInterrupt(test_bouton.testEnigme(), inter_enigme_bp, RISING);
  //---------initialisation des affichages
  Serial.printf("debut");
  affichage_demarrage();

}

void loop() {
//----------------Lecture des capteurs ------------------------
  int tension_A0 = ADS.readADC(0); //lumos       // Mesure de tension de la broche A0, par rapport à la masse
  int tension_A1 = ADS.readADC(1); //potar

//conversion de la valeur lu par le potar vers la varibale pos qui sera envoyé au servo
  pos = map(tension_A1, 0,25000, 0, 183);

//------------------ Test flag interruption bouton et touchsensor ------------------------
// interruption bouton
  if(flag_bouton == true){
    flag_bouton = false; //rabaisse le flag
    lasttimecpt = millis();
  //Serial.println(test_bouton.testEnigme());
  test_bouton.addCompteur() ;//add + 1 au compteur
  lcd.setCursor(0, 1);
  lcd.printf("Reponse BP: %d",test_bouton.getCompteur());
  }


// interruption touch sensor
  if(flag_touchb == true){
  flag_touchb = false; //rabaisse le flag
  test_touch.restart();
  test_bouton.setCompteur(0);
  affichage_demarrage();
  }

//--------------------affichage-----------------------------------
enigme_bouton = test_bouton.testEnigme();
enigme_light = test_light.testEnigme();
enigme_hum = test_hum.testEnigme();


int nb_e;

//gestion du nombre d'enigmes restantes
nb_e = 4 - (int)enigme_bouton -(int)enigme_hum - (int)enigme_light - (int)enigme_potar;
if((nb_e == 0)&&(nb_e != test_potar.getNb_enigme())){
  digitalWrite(D5, HIGH);
  delay(50);
  digitalWrite(D5, LOW);
  delay(20);
  digitalWrite(D5, HIGH);
  delay(20);
  digitalWrite(D5, LOW);
  }
  else if(test_potar.getNb_enigme() != nb_e){
  digitalWrite(D5, HIGH);
  delay(50);
  digitalWrite(D5, LOW);
  delay(50);
  }else if(nb_e == 1){
    enigme_potar = test_potar.testEnigme();
    myservo.write(pos);
    delay(150);
  }else if (nb_e == 0){
    myservo.write(pos); //utilisation du servo meme quand le jeu est fini
    delay(150);

  }

//----sauvegarde de l'ancienne valeur du nombre d'enigmes resolues
test_potar.setNb_enigme(nb_e);


//----------------Gère le timer du bouton ------------------------
  auto nowcpt = millis();
  if (nowcpt - lasttimecpt > DUREE_COMPTEUR){
  //lasttimecpt = nowcpt;
  test_bouton.setCompteur(0);
  lcd.setCursor(0, 1);
  lcd.print("                ");
  }

// -------------test affichage------------------------------------------------- 
  if(nb_e == 0){
    affichage_final();
   }else if(nb_e == 1){
    affichage_normal(nb_e);
   }
    else{
    affichage_normal(nb_e);
    }
}

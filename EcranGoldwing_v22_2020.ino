// UTFT_ViewFont (C)2014 Henning Karlsen
// web: http://http://www.rinkydinkelectronics.com/
// This program requires the UTFT library.


#include <UTFT.h>  // Pour les fonctions d'affichages.

#include <DS3231.h>  // Pour le module horloge.
#include <Wire.h>    // #include nécessaire pour l'horloge, RTClib en dépend.
#include <EEPROM.h>  // Pour mémoriser les totaux Km.


// Include et définition pour la sonde de température DHT22
//************************************************************
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

// #define DHTPIN A5      // Pin which is connected to the DHT sensor.
// #define DHTTYPE DHT22  // DHT 22 (AM2302)
// DHT_Unified dht(DHTPIN, DHTTYPE);
// uint32_t delayMS;

//*************************************************************

// Déclaration des fonts utilisées
//***********************************
extern uint8_t SmallFont[];  // 8x16
extern uint8_t BigFont[];    //font 16X16
extern uint8_t Font[];
extern uint8_t SevenSegNumFontPlusPlus[];  // 32x50
extern uint8_t SevenSeg_XXXL_Num[];        // 64x100 pixels
extern uint8_t GroteskBold32x64[];
extern uint8_t Grotesk24x48[];
extern uint8_t SixteenSegment64x96Num[];
extern uint8_t Arial_round_16x24[];
extern uint8_t Grotesk16x32[];
extern uint8_t Retro8x16[];

UTFT myGLCD(ILI9486, 38, 39, 40, 41);  // Modèle de l'écran utilisé 3,2' HVGA 480X320 pour Mega2560 .

DS3231 Clock;
bool Century = false;
bool h12;
bool PM;
byte ADay, AHour, AMinute, ASecond, ABits;
bool ADy, A12h, Apm;
byte annee, month, date, DoW, heure, minutes, seconde;

// Déclaration des variables.
//***********************************
unsigned long TempClignote = 1000;  // Valeur de pause 1 seconde.
unsigned long Temps;
unsigned long TempsPasser;
unsigned long TempAffichage;
unsigned long TempChange = millis();

float Speed = 0.00;
float Bat;  // mémoire pour la tension de charge/batterie

volatile int long NombrePulse = 0;

volatile boolean Pass = false;  // false en attentant le signal LOAD de début de trames.(Radio)
volatile int Lec = 0;
volatile int Trame = 0;
int Bits = 3;

int Version = 22;  // Ajout de la gestion du radio/K7.

int Pression2 = 0;
int xDate = 0, yDate = 4;    // Offset x et y pour l'affichage de la date
int xHeure = 0, yHeure = 0;  // Offset x et y pour l'affichage de l'heure
int xTemp = 0, yTemp = 0;    // Offset x et y pour l'affichage de la température externe
int MotoOn = 4;              // Pin digitale nr4 "ON" quand le contact est mis.
int PressionHuile = 7;       // Broche recevant le signal du détecteur de pression huile moteur.
int Bequille = 8;            // Broche recevant le signal du détecteur de la béquille latérale.
int MarcheArriere = A6;      // Broche recevant le signal du détecteur de la Position marche ar)o i rière.
int GearN = A9;              // Broche recevant le signal de la position Neutre.
int Gear2 = 10;              // Broche recevant le signal de la position 2ieme vitesse.
int Gear3 = 11;              // Broche recevant le signal de la position 3ieme vitesse.
int Gear4 = 12;              // Broche recevant le signal de la position 4ieme vitesse.
int Gear5 = 13;              // Broche recevant le signal de la position 5ieme vitesse.
int SecuriteCoffre = A7;     // Broche recevant un signal 5v si un coffre est mal fermé.
int SecuriteSwitch = A8;     // Broche recevant un signal 5v si le switch coupure moteur est actif.
int Cphare = A11;            // Gestion du phare.
int TotalImpulsion;          // Variable de travail des pulses du compteur kilométrique.
int PhotoRes = A3;           // Broche recevant la valeur de la photo-résistance.
int TotalEntretien;          // variable de travail.
int Recharge = 5000;         // Valeur de recharge pour l'intervale des entretiens.

// Variable pour le décodage Radio/K7
//*********************************************************
char Chiffre[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' };  // Pour la conversion int/char
//String Dfreq;
byte Mode_Radio = 1;  // 0= AM 1= FM
String Sfrequence;
int Frequence = 0;
int Mode = 1;  // 0= AM 1= FM
byte Mille, Centaine, Dizaine, Unite, ByteRead;
int Index, Index1;
boolean Radio = true, Mute = false;
byte Code, Data, Frequ;
//****************************************************************


String VitSelect = "0";  // initialisation de la variable contenant la valeur à afficher du sélecteur de vitesse.
String VitSel;           // Mémoire de la vitesse boite en cours.

word CouleurAff;
word CouleurFond;
word Couleur;
word CouleurEnCours;
word CouleurCompteur;

boolean Veille = false;  // Veille en service.
boolean Nuit = false;
boolean EcranFait = false;
boolean Initialisation = false;  // Drapeau d'activation du mode Debug.
boolean Allume = true;
boolean Info = false;
boolean SetInfo = false;  // Balise mémoire pour l'effacement de la zone de texte des alarmes.
boolean SpeedOk = false;  // Balise signal que la valeur de la vitesse est à jour.
boolean EepromTrue = true;
boolean Mfrequence = false;  //
boolean AlarmeEntretien = true;

byte Message[] = { 0, 0, 0, 1, 0, 7, 8 };

// Variable de la sonde température
//************************************
float Temperature;
float SensorTemp;

// Variable pour le compteur journalier
//**************************************
float TotalJournalier;  // Variable mémoire du total compteur journalier.
float TotalCompteur;    // Variable mémoire du total des kilomètres.
float EntretienHuile;   // Variable mémoire des kilomètres du prochain entretien .

//***************************************************************************************************
void setup() {
  // pinMode(A0,INPUT); // pin en entrée (température du radiateur).
  // pinMode(A1,INPUT); // pin en entrée (tension de la batterie).
  // pinMode(A2,INPUT); // pin en entrée (niveau de l'essence).
  // pinMode(A3,INPUT); // pin en entrée, LDR.pour détecter jour/nuit.
  // pinMode(A4,INPUT); // pin en entrée, pression suspension arrière.
  // pinMode(A5,INPUT); // pin en entrée,sonde de température extérieur.
  // pinMode(A6,INPUT_PULLUP); // pin en entrée, Contact de la marche arrière.
  // pinMode(A7,INPUT_PULLUP); // pin en entrée, Sécurité coffre.
  // pinMode(A8,INPUT); // pin en entrée, switch coupure moteur.
  pinMode(A9, INPUT); // pin en sortie, ventilateurs des radiateurs. (via relais, pas raccordé).
  // pinMode(A10,OUTPUT); // pin en sortie, activation de l'éclairage des valises la nuit.(via relais).
  // pinMode(A11,OUTPUT); // pin en sortie, feux de croisement.(via relais).
  // pinMode(A12,INPUT); //
  // pinMode(A13,INPUT); //
  //pinMode(A14,INPUT); // Serial3 Tx pour Data radio
  //pinMode(A15,INPUT); // Serial3 Rx pour Data radio

  // pinMode(2,INPUT); //pin en entrée, reçoit les impulsions du compteur de vitesse.
  // pinMode(3,INPUT_PULLUP); //pin en entrée n.c.
  // pinMode(4,INPUT_PULLUP); //pin en entrée, reçoit la tension contact ON.
  // pinMode(5,INPUT_PULLUP); //pin en entrée réglage heure
  // pinMode(6,INPUT_PULLUP); //pin en entrée réglage minute
  // pinMode(7,INPUT_PULLUP); //pin en entrée, reçoit la tension manocontact de la pression huile moteur.
  // pinMode(8,INPUT_PULLUP); //pin en entrée, reçoit la tension du contact de la Béquille latérale.

  //pinMode(Gear1,INPUT_PULLUP);  // Il n'y a pas de position de contact pour la 1ere vitesse.
  pinMode(9, INPUT);  // Entrée vitesse sélectionnée Neutre (br9)
  pinMode(10, OUTPUT);  // Entrée vitesse sélectionnée Neutre (br9)
  // pinMode(Gear2,INPUT_PULLUP);  // Entrée vitesse sélectionnée 2iem (br10)
  // pinMode(Gear3,INPUT_PULLUP);  // Entrée vitesse sélectionnée 3iem (br11)
  // pinMode(Gear4,INPUT_PULLUP);  // Entrée vitesse sélectionnée 4iem (br12)
  // pinMode(Gear5,INPUT_PULLUP);  // Entrée vitesse sélectionnée 5iem (br13)

// initialize serial communication:
  // Serial.begin(9600);

  
  Wire.begin();  // Instanciation de la communication I2C interface.
  // dht.begin();   // Instanciation de la communication de la sonde température DHT22

  // Initialisation de la date par programme, pas de mode manuel.
  //
  //A faire à la première programmation de l'horloge
  /*******************************************************************
   Clock.setDoW(4);    //Initialise le jour de la semaine.
   Clock.setDate(9);  //Initialise le jour.
   Clock.setMonth(4);  //Initialise le mois.
   Clock.setYear(20);  //Initialise l'année.
 */

  // Serial3.begin(115200);  // communication avec la radio ( Nano )
  Serial.begin(115200);   // Sélectionne la vitesse du port série pour le debug pc.

  myGLCD.InitLCD(PORTRAIT);            // Initialisation de l'écran en mode portrait.
  myGLCD.clrScr();                     // Efface l'écran.
  myGLCD.setBackColor(150, 250, 100);  // Initialise la couleur du fond pour l'écriture des caractères.
  CouleurFond = myGLCD.getBackColor();
  myGLCD.setColor(VGA_NAVY);  // Initialise la couleur de l'écriture des caractères.
  CouleurAff = myGLCD.getColor();
  CouleurCompteur = (0, 255, 0);

  // digitalWrite(A9,0);  // Désactive les ventilateurs des radiateurs.
  // digitalWrite(A10,1);  // Désactive l'éclairage nuit.
  // digitalWrite(Phare,0);  // Désactive le phare.

  // Initialisation des interruptions
  // cli();  // Désactive toutes les interruptions

  // Initialisation de l'interruption nr0
  //*****************************************
  // attachInterrupt(digitalPinToInterrupt(2), PulseCompteur, FALLING);  // La fonction PulseCompteur sera appelée à chaque interruption (La Broche 2, reçoit les impulsions du compteur de vitesse).


  // Timer pour la lecture de la vitesse
  //*************************************
  // TCCR1A = 0;
  // TCCR1B = 0b00001100;
  // TIMSK1 = 0b00000010;
  // TCNT1 = 0;
  // OCR1A = 31250;  // Pour une interruption toutes les 1/2 secondes.

  // sei();  // Autorise toutes les interruptions.

  // TotalJournalier = eeprom_read_float(0);  // Lecture dans l'eeprom de la valeur sauvée du total km journalier.
  // TotalCompteur = eeprom_read_float(4);    // Lecture dans l'eeprom de la valeur sauvée du total km .
  // //Speed = eeprom_read_float(8);         // Lecture dans l'eeprom de la valeur sauvée du compteur d'impulsion vitesse .
  // EntretienHuile = eeprom_read_float(12);  // Lecture du nombre de kilomètres avant prochain entretien.
  // if (EntretienHuile <= 500) { AlarmeEntretien = true; }
  // // eeprom_write_float( 4,175447.464);

  // while (!Serial3) {
  //   ;  // Attente d'initialisation du port Serial3
  // }

  // Serial3.write(1);  // Demande l'envoi des codes radio/K7 (Arduino Nano)




  // Serial.print(" Ecran Goldwing 1500. Version :  ");
  // Serial.println(Version);
  // Serial.println("");

  // Serial.print(" Total km compteur :  ");
  // Serial.println(TotalCompteur);
  // Serial.println("");

  // Serial.print(" Total km journalier :  ");
  // Serial.println(TotalJournalier);
  // Serial.println("");

  // **** Fin de l'initialisation.*
  //*******************************
}
// Fin du Setup.


int buttonState = 0;        // current state of the button
int lastButtonState = 0;

//***************************************************************************************************
void loop() {
  FondEcran();

  int A1x = 25, A1y = 180, A2x = 25, A2y = 232;

myGLCD.setFont(BigFont);  //font 16X16

  buttonState = digitalRead(9);
  myGLCD.print(String(buttonState), A1x, 100);

  int valeur = analogRead(A9);
  float tension = valeur * (5.0 / 1023.0);

  myGLCD.print(String(tension), A1x, 300);
  
  if (digitalRead(A9) == LOW) {
    myGLCD.print("low", A2x, A2y);
    // digitalWrite(10, HIGH);
  } else {
    myGLCD.print("high", A1x, A1y);
    // digitalWrite(10, LOW);
  }

  if (buttonState != lastButtonState) {
    myGLCD.print("true", A1x, A1y);
    // digitalWrite(10, HIGH);
  } else {
    myGLCD.print("false", A1x, A1y);
    // digitalWrite(10, LOW);
  }

  // myGLCD.print(String(analogRead(A9)), A2x, A2y);

  // if ((analogRead(A9) == false)) {
  //   myGLCD.print("Boite s/", A1x, A1y);  // Affichage de l'alarme Mettre BOITE SUR NEUTRE à la positio X,Y.
  //   myGLCD.print("Neutre! ", A2x, A2y);  //
  //   // Info=true;
  //   // goto PasseAlarme;
  // } else {
  //   myGLCD.print("Boite s/", A1x, A1y);         // Affichage de l'alarme Mettre BOITE SUR NEUTRE à la positio X,Y.
  //   myGLCD.print("Pas au Neutre! ", A2x, A2y);  //
  // }


  // // ** Début du LOOP.*
  // //*******************

  // //**************************************************************************************************************
  // // Si à la mise du contact, le réglage de l'heure est tourné vers le haut, le mode d'affichage debug est activé.*
  // //**************************************************************************************************************
  //   if (Initialisation == false)  // test si premier passage.
  //                    {
  //                         Initialisation = true; // Valide le premier passage.
  //                         if ((digitalRead(5) == false) && ( digitalRead(MotoOn) == false)) // Si le réglage de l'heure est actionné lors de la mise sur contact ON, passage en mode debug.
  //                                                   {
  //                                                       WriteStatut(); // Affichage du mode debug; en boucle infinie, jusqu'au redémarrage de l'interface.
  //                                                   }
  //                       if (digitalRead(6) == false)  // Si le réglage de l'heure est poussé vers le bas lors de la mise sous tension,
  //                                // les kilomètres d'entretien sont remis à jour avec la valeur de "Recharge".
  //                         {
  //                           EntretienHuile = TotalCompteur + Recharge ; // remet la valeur pour le prochain entretien
  //                           //AlarmeEntretien = false; // Désactive l'affichage de l'alarme
  //                           eeprom_write_float( 12,EntretienHuile); // Sauve la valeur en eeprom.
  //                         }

  // // Initialisation du mode jour ou nuit.
  // //****************************************
  //                        if (analogRead(PhotoRes) > 15)  // Si la valeur est plus grande que 15, sélection de l'écran jour.
  //                                             {
  //                                               Nuit=false;
  //                                             }

  //                         if (analogRead(PhotoRes) < 16 )  // Si la valeur est plus petite que 16, sélection de l'écran nuit.
  //                                             {
  //                                               Nuit=true;

  //                                             }
  //                        EcranFait=false; // Force le renouvellement de l'affichage du fond d'écran statique.
  //                        FondEcran();
  //                    //* Mise à jour du niveau d'essence et de la température extérieur.
  //                    //********************************************************************
  //                       Essence(); // Affichage du niveau d'essence.
  //                       afficheTemp(100, 60);// affichage de la température extérieur.
  //                    }

  // //************************
  // // Passage en mode actif.*
  // //************************
  // // Moto ON.*
  //   //********

  //   if (( digitalRead(MotoOn) == false) && ( Veille == true) ) // Si le contact est sur ON, et que le mode veille est actif, désactivation du mode veille.
  //   {
  //     myGLCD.fillScr(VGA_BLUE); // (VGA_TEAL)
  //     myGLCD.setBackColor(VGA_BLUE); //(VGA_TEAL) Sélection de la couleur pour le fond d'écran.
  //     myGLCD.setColor(CouleurAff);  // Sélection de la couleur pour l'écriture des caractères.
  //     Veille = false;  // Supprime le mode veille.
  //     EcranFait= false;  // Provoque le renouvellement de l'affichage.
  //     FondEcran();  // Renouvellement de l'affichage du fond d'écran statique.
  //     VitSel = 7; // Pour obliger à mettre à jour la vitesse sélectionnée.
  //   }


  // //**********************************************************************************************
  // // Si le contact est sur ON et le mode veille NON ACTIVEE ( veille = contact sur accessoires ) *
  // //**********************************************************************************************
  // // MODE ACTIF.( = Moteur en marche) *
  // //*********************************
  //   if (( digitalRead(MotoOn) == false) && ( Veille == false) )
  //   {

  //                 if (TempChange < millis() ) // Passage toutes les secondes.
  //                 {
  //                         Ldr();                // Active le mode nuit ou jour. ( Ecran et gestion de l'éclairage des valises ).

  //                         Phare();             // Gestion de l'allumage du feux de croisement. Allume le phare si la pression d'huile est présente.
  //                         ReadDS3231();        // Affichage de la date, heure.
  //                         Essence();           // Affichage du niveau d'essence.
  //                         afficheTemp(100, 60);// affichage de la température extérieur.
  //                         Pression();          // Affichage de la pression de l'amortisseur.
  //                         Batterie();          // Affichage de la tension de la batterie.
  //                         Radiateur();         // Affichage de la température du radiateur.

  //                         TempChange= millis() +1000;
  //                 }
  //                         Alarme();            //  Affichage d'une alarme ( béquille latérale, marche arrière, pression d'huile moteur, Coffre ouvert, switch coupure moteur).
  //                               if (( Info == false ) && (SpeedOk == true ))  // Si pas d'alarme affichée et vitesse mise à jour.
  //                                   {
  //                                    Vitesse(); //  Affichage de la vitesse.
  //                                    Gear();    // Affichage de la vitesse sélectionnée.
  //                                    }






  // // Mise à zéro du compteur journalier. Bouton de réglage de l'heure vers le bas.
  // //***********************************************************************************

  // if (digitalRead(6) == false)
  //   {
  //     TotalJournalier = 0; // Mise à zéro du compteur de kilomètres journalier.
  //     TotalImpulsion = 0;   // Mise à zéro du compteur d'impulsion.
  //     eeprom_write_float (0,TotalJournalier); // Mise à jour en eeprom à l'adresse 0x00 de la valeur du totalisateur journalier.
  //   }

  //  }
  // // Fin du mode Actif.


  // /*
  // ******************************************************************************************
  // ** Si le contact est sur acc. et le mode veille est non actif, activation du mode veille. *
  // *******************************************************************************************
  // Le mode veille consiste en un écran statique.  *
  // Permet la mise à l'heure de l'horloge.         *
  // ************************************************
  // *Initialisation de l'écran pour le MODE VEILLE.*
  // ***********************************************
  // */

  // Veille:

  //   if (( digitalRead(MotoOn) == true) && ( Veille == false) )  // si le mode veille n'est pas encore installé.
  //       {
  //        myGLCD.clrScr(); // Efface l'écran.
  //        Veille = true; // Mode veille est activé.
  //        myGLCD.fillScr(0,0,0);
  //        myGLCD.setBackColor(0,0,0); // Fond d'écran noir.
  //        myGLCD.setColor(VGA_WHITE); // Couleur d'affichage des caractères en Blanc.
  //       }

  //   //*********************************************************************************************************
  //  if (( digitalRead(MotoOn) == true) && ( Veille == true) )  // Si contact sur acc. et veille activée.
  //      {
  //        HorlogeVeille(); // Affichage de la date,heure et température.

  // //***********************
  // // Réglage de l'horloge.*
  // //***********************
  //  // Si le bouton de réglage est tourné: vers le haut: réglage de l'heure, vers le bas: les minutes.
  //  //
  //        if (digitalRead(5) == false)  // Mise à jour de l'heure.
  //          {
  //              heure=   Clock.getHour(h12, PM); // Lecture de la mémoire de l'heure.
  //              heure=heure+1; // Incrémente la valeur.

  //             if (heure >= 24){ heure=0;} // Si dépassement de 24h.on revient à zéro.

  //             Clock.setHour(heure); // Enregistrement de la nouvelle valeur dans la mémoire de l'heure.
  //          }
  //        if (digitalRead(6) == false) // Mise à jour des minutes.
  //          {
  //            minutes= Clock.getMinute(); // // Lecture de la mémoire des minutes.
  //            minutes=minutes+1; // Incrémente la valeur.

  //            if (minutes >= 59) {minutes=1;} // Si dépassement de 59,on revient à zéro.

  //            Clock.setMinute(minutes); // Enregistrement de la nouvelle valeur dans la mémoire des minutes.
  //          }


  //     }
  // //******************************************************************************************************************
  // // Gestion de la RADIO/K7
  // //******************************************************************************************************************
  //                              // Lecture & mémorisation des codes de la radio/K7
  //                       if ( Serial3.available() >=2 ) // test pour la réception du code et du data
  //                          {

  // Lecture:                   Code = Serial3.read(); // Lecture du code

  //                            if ( Code == 0 || Code> 14){ goto Lecture;}
  //                           //Serial.print ("Code = "); Serial.print(Code);

  //                            Data = Serial3.read();  // Lecture de la donnée ou poid fort du byte de la fréquence
  //                          // Serial.print ("   --> "); Serial.println(Data);

  //                            if (Code == 14){
  //                                             Frequ = Serial3.read() ;  // Lecture du poid faible du byte de la fréquence
  //                                            // Serial.print ("Code = 15-  --> "); Serial.println(Frequ);
  //                                           }

  //                          Radio_lecture();           // Lecture et affichage des données envoyées par le radio/K7
  //                          }
}

// FIN DE LA FONCTION LOOP.



//**********************************************************************************************************************
// Activation de l'allumage du phare (feux de croisement), si la pression d'huile est présente..
//**********************************************************************************************************************
void Phare() {

  if (digitalRead(PressionHuile) == true)  // Si il n'y a pas de pression d'huile, la moto est inactive.
  {
    digitalWrite(Cphare, 0);  // Désactive le feux de croisement.
  } else                      // Si il y a pression d'huile, la moto est active.
  {
    digitalWrite(Cphare, 1);  // Active le feux de croisement.
  }
}
//**********************************************************************************************************************
// Affichage des alarmes dans la zone d'affichage de la vitesse.
// Si il n'y a pas de déplacement, l'affichage de l'alarme est statique.
// Si il y a déplacement, celui-ci alterne avec l'affichage de la vitesse.
//**********************************************************************************************************************

void Alarme() {
  if (digitalRead(PressionHuile) == true) { goto StopClignote; }  // Si pas de moteur en marche, pas de clignotement d'alarme.

  Clignote();                                //
  if (Allume == true) { goto PauseCligno; }  // Si une alarme, autorise l'affichage de la vitesse par intermitence.

StopClignote:
  if (digitalRead(Bequille) == true || digitalRead(MarcheArriere) == true || digitalRead(PressionHuile) == true
      || digitalRead(SecuriteCoffre) == false || digitalRead(SecuriteSwitch) == false || Bat < 11)  // teste si une alarme active.

  {
    AffKm();  // Affichage du totalisateur Km journalier.

    if (SetInfo == false) { ClearInfo(); }  // Efface la zone pour l'affichage du texte de l'alarme.
    Couleur = myGLCD.getColor();            // Mémorise la couleur d'affichage.
    myGLCD.setFont(Grotesk24x48);           //
    myGLCD.setColor(VGA_RED);               // Passe en rouge pour la couleur des caractères.

    int A1x = 25, A1y = 180, A2x = 25, A2y = 232;  // Coordonnées A1x-y= texte supérieur/ A2x-y= texte inférieur.

    //*******************************************************************************************************

    if (digitalRead(MarcheArriere) == true)  //
    {
      myGLCD.print("Marche  ", A1x, A1y);  // Affichage de l'alarme MARCHE ARRIERE à la positio X,Y.
      myGLCD.print("Arriere ", A2x, A2y);  // Affichage ARRIERE à la positio X,Y.
      Info = true;
      goto PasseAlarme;
    }

    if ((digitalRead(SecuriteSwitch) == false)) {
      myGLCD.print("STOP    ", A1x, A1y);  // Affichage de l'alarme Switch (coupe circuit actif) à la positio X,Y.
      myGLCD.print("Switch! ", A2x, A2y);  //
      Info = true;
      goto PasseAlarme;
    }

    if (digitalRead(SecuriteCoffre) == false) {
      myGLCD.print("Coffre  ", A1x, A1y);  // Affichage de l'alarme COFFRE OUVERT à la positio X,Y.
      myGLCD.print("ouvert! ", A2x, A2y);  //
      Info = true;
      goto PasseAlarme;
    }

    if ((digitalRead(GearN) == false) && (digitalRead(PressionHuile) == true)) {
      myGLCD.print("Boite s/", A1x, A1y);  // Affichage de l'alarme Mettre BOITE SUR NEUTRE à la positio X,Y.
      myGLCD.print("Neutre! ", A2x, A2y);  //
      Info = true;
      goto PasseAlarme;
    }

    if (digitalRead(Bequille) == true)  //
    {
      myGLCD.print("Bequille", A1x, A1y);  // Affichage de l'alarme BEQUILLE à la positio X,Y.
      myGLCD.print("Laterale", A2x, A2y);  // Affichage LATERALE à la positio X,Y.
      Info = true;
      goto PasseAlarme;
    }
    if (Bat < 11) {
      myGLCD.print("Charge  ", A1x, A1y);  // Affichage de l'alarme Batterie
      myGLCD.print("Faible  ", A2x, A2y);  // Faible à la positio X,Y.
      Info = true;
      goto PasseAlarme;
    }

    if (digitalRead(PressionHuile) == true)  //
    {
      myGLCD.print("Pression", A1x, A1y);  // Affichage de l'alarme PRESSION D'HUILE à la positio X,Y.
      myGLCD.print("Huile   ", A2x, A2y);  // Affichage HUILE à la positio X,Y.
      Info = true;
      goto PasseAlarme;
    }
    if (AlarmeEntretien == true) {
      myGLCD.print("Changer ", A1x, A1y);  // Affichage de l'alarme PRESSION D'HUILE à la positio X,Y.
      myGLCD.print(" Huile  ", A2x, A2y);  // Affichage HUILE à la positio X,Y.
      Info = true;
    }
PasseAlarme:
    myGLCD.setColor(Couleur);  // Rétablit la couleur d'affichage des caractères sauvés initialement.
  } else                       // Si pas ou plus d'alarme.
PauseCligno:
  {
    if (Info == true)  // Si il y a eu une alarme affichée, restauration pour l'affichage de la vitesse.
    {
      ClearInfo();                    // Efface la zone texte des alarmes.
      myGLCD.setFont(BigFont);        // Font 16X16
      myGLCD.print("KMH", 250, 180);  // Repère N
      Info = false;
      SetInfo = false;
      VitSel = 7;  // Pour obliger à mettre à jour la vitesse sélectionnée.
    }
  }
}

//**********************************************************************************************************************
void ClearInfo()
//**********************************************************************************************************************
{
  // Efface la zone d'affichage compteur.
  //**************************************
  myGLCD.setColor(CouleurFond);
  myGLCD.fillRect(16, 173, 168, 289);  // Efface la zone d'affichage de vitesse/information.
  myGLCD.setColor(CouleurAff);         // Rétablit la couleur d'affichage des caractères sauvés initialement.
  SetInfo = true;                      // balise mémoire indiquant la zone de texte effacée.
  VitSel = 7;                          // Pour obliger à mettre à jour la vitesse sélectionnée.
}

//**********************************************************************************************************************
void FondEcran()  // Affichage des séparateurs et textes

{
  if (Veille == true) return;  // En mode veille, l'affichage est différend.

  //******* MODE NUIT *************
  //*******************************

  if ((Nuit == true) && (EcranFait == false))  // Si la luminosité est faible,modification de l'écran sur mode nuit.
  {
    myGLCD.setBackColor(VGA_BLACK);       // Initialise la couleur du fond pour l'écriture des caractères.
    CouleurFond = myGLCD.getBackColor();  // Mémorise la couleur du fond.
    myGLCD.setColor(VGA_WHITE);           // Initialise la couleur de l'écriture des caractères.
    CouleurAff = myGLCD.getColor();       // Mémorise la couleur d'affichage des caractères.

    myGLCD.fillScr(CouleurFond);       // Noir
    myGLCD.setBackColor(CouleurFond);  // Fond d'écran noir
    myGLCD.setColor(CouleurAff);       // Couleur des caractères
    VitSel = 7;                        // Pour obliger à mettre à jour la vitesse sélectionnée.
  }

  //******* MODE JOUR *************
  //*******************************
  if ((Nuit == false) && (EcranFait == false))  // Si luminosité forte, modification de l'écran sur mode jour.
  {
    myGLCD.setBackColor(150, 250, 100);   // Initialise la couleur du fond pour l'écriture des caractères.
    CouleurFond = myGLCD.getBackColor();  // Mémorise la couleur du fond.
    myGLCD.setColor(VGA_BLACK);           // Initialise la couleur de l'écriture des caractères.
    CouleurAff = myGLCD.getColor();       // Mémorise la couleur d'affichage des caractères.
    myGLCD.fillScr(CouleurFond);          //
                                          //myGLCD.setBackColor(CouleurFond); //
    // myGLCD.setColor(CouleurAff);  // Couleur des caractères
    VitSel = 7;  // Pour obliger à mettre à jour la vitesse sélectionnée.
  }

  // myGLCD.setFont(BigFont);  //font 16X16

  // myGLCD.print("KMH", 250, 180);       // Repère N
  // myGLCD.print("CHARGE", 35, 330);     // Affichage du nom de l'indicateur à la position X,Y (repère Q)
  // myGLCD.print("TEMP EXT", 175, 330);  // Affichage du nom de l'indicateur à la position X,Y (repère R)
  // myGLCD.print("K= ", 25, 300);        // Affichage du nom de l'indicateur kilo de pression amortisseur à la position X,Y (repère P)
  // myGLCD.print("T.MOTEUR", 20, 420);   // Affichage du nom de l'indicateur à la positio X,Y (repère V)
  // myGLCD.print("ESSENCE", 180, 420);   // Affichage du nom de l'indicateur à la positio X,Y (repère W)


  // myGLCD.drawRect(15, 170, 315, 43);   // Cadre 1
  // myGLCD.drawRect(15, 290, 315, 325);  // Cadre 2
  // myGLCD.drawRect(15, 350, 315, 413);  // Cadre 3
  // myGLCD.drawRect(15, 440, 315, 478);  // Cadre 4

  // myGLCD.drawLine(160, 290, 160, 478);  // Ligne E
  // myGLCD.drawLine(15, 40, 15, 478);     // Ligne F
  // myGLCD.drawLine(315, 478, 315, 43);   // Ligne G
  // myGLCD.drawLine(190, 170, 190, 43);   // Ligne K
  // myGLCD.drawLine(190, 75, 315, 75);    // Ligne K1 Séparation supérieure radio
  // myGLCD.drawLine(190, 135, 315, 135);  // Ligne K8 Séparation inférieure radio

  // myGLCD.print("Km", 280, 300);  // Affichage de l'indicateur Km journalier à la positio X,Y (repère w1)

  /*
 // Section supprimée, remplacée par l'affichage du compteur de kilomètre journalier.
 //***********************************************************************************
 
  // Drapeau Belge
  // ******************

  // Encadrement
  // *************  
  myGLCD.setColor(VGA_WHITE); 
  myGLCD.drawLine( 199, 293, 199, 321); // Encadrement ligne gauche 
  myGLCD.drawLine( 199, 293, 271, 293); // Encadrement ligne superieur 
  myGLCD.drawLine( 199, 321, 271, 321); // Encadrement ligne inferieur 
  myGLCD.drawLine( 271, 293, 271, 321); // Encadrement ligne droite 

  // Drapeau
  // ************
  myGLCD.setColor(VGA_BLACK);
  myGLCD.fillRect(200,295,245,320); // Carré noir
  myGLCD.setColor(VGA_YELLOW);
  myGLCD.fillRect(226,295,250,320); // Carré jaune
  myGLCD.setColor(VGA_RED);
  myGLCD.fillRect(251,295,270,320); // Carré rouge
  myGLCD.setColor(CouleurAff);  // Couleur des caractères 

  */

  // Mise à jour du niveau d'essence et de la température extérieur.
  //****************************************************************

  // Essence();  // Affichage du niveau d'essence.

  // afficheTemp(100, 60);  // affichage de la température extérieur.
  // VitSel = 7;            // Pour obliger à mettre à jour la vitesse sélectionnée.
  // Gear();                // Affichage de la vitesse sélectionnée.

  EcranFait = true;  // Indique que l'affichage a été renouvellé, pour ne pas y repasser sans cesse.
}

//***********************************************************************************
// Sélection de l'écran en mode jour ou nuit en fonction de la résistance de la LDR.*
// Gestion de l'éclairage des valises.                                              *
//***********************************************************************************
void Ldr() {

  // Mode jour
  if ((analogRead(PhotoRes) > 800) && (Nuit == true))  // Si la valeur est plus grande que 800, et que c'est en mode nuit,sélection de l'écran jour.
  {
    Nuit = false;
    EcranFait = false;  // Force le renouvellement de l'affichage du fond d'écran statique.
    FondEcran();
    digitalWrite(A10, 1);  // Extinction des éclairages valises.
  }



  // Mode nuit
  if ((analogRead(PhotoRes) < 600) && (Nuit == false))  // Si la valeur est plus petite que 600, et que c'est en mode jour, sélection de l'écran nuit.
  {
    Nuit = true;
    EcranFait = false;  // Force le renouvellement de l'affichage du fond d'écran statique.
    FondEcran();
    digitalWrite(A10, 0);  // Allumage des éclairages valises.
  }


  // Section désactivée pour avoir l'éclairage des valises actif lors de l'arrêt du moteur (en mode nuit).
  //*******************************************************************************************************
  /*  if ((Nuit== true) && ( digitalRead(PressionHuile)==false))
 
                                              {  
                                                digitalWrite(A10,0); // Allumage des éclairages valises.
                                              }
                                            else
                                              {
                                                digitalWrite(A10,1); // extinction des éclairages valises.
                                              } 
*/
}

//*****************************************************************************************
void Essence()  // Lecture et affichage du niveau d'essence.

// A optimiser pour stabiliser l'affichage.
//*******************************************

{
  myGLCD.setFont(BigFont);          //font 16X16
  int sensorFuel = analogRead(A2);  // Lecture de la valeur sur l'entrée analogique A2
  // Serial.print(" essence = ");
  //Serial.println (sensorFuel);

  int bulleFuel = 180;  // Position en X de l'affichage de la première barre.
  int LigneX = 168, LigneY = 445;
  int compteurFuel = 12;  // Pour 12 barres.
  int Fuel = 0;
  word Couleur = myGLCD.getColor();  // Mémorise la couleur d'affichage.

  if (sensorFuel < 45) {
    Fuel = 12;
    goto FinFuel;
  }  // Réservoir plein entre 22 et 18 litres
  if (sensorFuel < 57) {
    Fuel = 11;
    goto FinFuel;
  }  // Reste entre 16 et 15 litres
  if (sensorFuel < 68) {
    Fuel = 10;
    goto FinFuel;
  }  // Reste entre 14 et 13 litres
  if (sensorFuel < 79) {
    Fuel = 9;
    goto FinFuel;
  }  // Reste entre 13 & 12 litres
  if (sensorFuel < 85) {
    Fuel = 8;
    goto FinFuel;
  }  // Reste entre 12 & 11 litres
  if (sensorFuel < 91) {
    Fuel = 7;
    goto FinFuel;
  }  // Reste entre 11 & 10 litres
  if (sensorFuel < 102) {
    Fuel = 6;
    goto FinFuel;
  }  // Reste entre 10 & 9 litres
  if (sensorFuel < 113) {
    Fuel = 5;
    goto FinFuel;
  }  // Reste entre 9 & 8 litres
  if (sensorFuel < 124) {
    Fuel = 4;
    goto FinFuel;
  }  // Limite de réserve ( reste +- 7 Litres )
  if (sensorFuel < 135) {
    Fuel = 3;
    goto FinFuel;
  }  // Reste entre 6 & 5 litres
  if (sensorFuel < 147) {
    Fuel = 2;
    goto FinFuel;
  }  // Reste entre 4 & 3 litres
  if (sensorFuel < 155) {
    Fuel = 0;
    goto FinFuel;
  }  // 3 à 4 litres

FinFuel:


  for (compteurFuel; compteurFuel >= 1; --compteurFuel)  // Boucle d'affichage du nombre de barre.
  {
    if (Fuel >= 1)  // Si au moins une barre à afficher.
    {
      if (compteurFuel > 5) myGLCD.setColor(CouleurAff);  //
      if (compteurFuel > 8) myGLCD.setColor(255, 0, 0);   // Couleur rouge.

      myGLCD.fillRect(LigneX, LigneY, LigneX + 7, LigneY + 27);  // Affichage de la barre à la positio X,Y
    } else {
      if (Fuel < 1)  // Si compteur barre est à 0, on efface les emplacements
      {
        CouleurFond = myGLCD.getBackColor();                           // Capte la couleur du fond
        myGLCD.setColor(CouleurFond);                                  // Assigne la couleur de l'affichage
        myGLCD.fillRect(LigneX, LigneY, (LigneX + 7), (LigneY + 27));  // Effacement de la barre à la positio X,Y
        myGLCD.setColor(CouleurAff);                                   // Assigne la couleur de l'affichage

        myGLCD.drawRect(LigneX, LigneY, (LigneX + 7), (LigneY + 27));  // Affiche une barre vide à la positio X,Y
        myGLCD.setColor(Couleur);                                      // Rétablit la couleur d'affichage
      }
    }
    Fuel = Fuel - 1;

    LigneX = LigneX + 12;  // Incrémente à la position suivante en X
  }
  myGLCD.setColor(Couleur);  // Rétablit la couleur d'affichage.
}

//**********************************************************************************************************************
void Radiateur()  // Lecture et affichage de la température du radiateur.

// A optimiser en fonction de la température du capteur de température.
//**********************************************************************
{

  if (Veille == true) { return; }  // Si mode veille, pas d'affichage de la température des radiateurs.

  int sensorTemp = analogRead(A0);  // Lecture de la valeur sur l'entrée analogique A0

  //Serial.print(" température radiateur = ");
  //Serial.println (sensorTemp);
  //Serial.println ("");

  int Bulle = 0;
  Couleur = myGLCD.getColor();
  int bulleTemp = 30;    // Position en X de l'affichage de la première bulle
  int compteurTemp = 5;  // Pour 5 bulles

  if (sensorTemp > 400)  // Température froid
  {
    Bulle = 0;
  }
  if (sensorTemp < 70)  // Température Mid
  {
    Bulle = 1;
  }
  if (sensorTemp < 30)  // Température dans la normal
  {
    Bulle = 2;
  }
  if (sensorTemp < 20) {
    Bulle = 3;
  }
  if (sensorTemp < 10)  // Température tres chaud = 16 ohms
  {
    Bulle = 4;
  }
  if (sensorTemp < 5)  // Température tres chaud =
  {
    Bulle = 5;
  }

  if (Bulle < 5)  // Test si la température est dans la gamme admissible.
  {
    myGLCD.setColor(CouleurAff);  // Couleur d'affichage normal.
    // digitalWrite(A9,1);  // Désactive les ventilateurs de refroidissement des radiateurs.
  } else  // Si la température des radiateurs est trop haute, que le moteur tourne et déplacement + de 20Kmh.
  {

    myGLCD.setColor(VGA_RED);  // Couleur d'affichage = rouge.
    // if ( digitalRead(PressionHuile)== true && Speed >= 20)
    // {
    //  digitalWrite(A9,0);  // Active les ventilateurs de refroidissement des radiateurs.
    // }
    // else { digitalWrite(A9,1);}
  }


  for (compteurTemp; compteurTemp >= 1; --compteurTemp)  // Boucle d'affichage du nombre de bulle.

  {
    if (Bulle >= 1)  // Si au moins une bulle à afficher
    {
      myGLCD.fillCircle(bulleTemp, 460, 10);  // Affichage de la bulle à la positio X,Y
    } else if (Bulle <= 1)                    // Si compteur bulle est à 0, on efface les emplacements
    {
      CouleurEnCours = myGLCD.getColor();     // Mémorise la couleur d'affichage en cours
      CouleurFond = myGLCD.getBackColor();    // Capte la couleur du fond.
      myGLCD.setColor(CouleurFond);           // Assigne la couleur de l'affichage
      myGLCD.fillCircle(bulleTemp, 460, 10);  // Efface avec la couleur du fond la bulle à la positio X,Y
      myGLCD.setColor(CouleurAff);            // Couleur du fond
      myGLCD.drawCircle(bulleTemp, 460, 10);  // Affiche la bulle vide à la positio X,Y
      myGLCD.setColor(CouleurEnCours);        // Rétablit la couleur d'affichage d'origine
    }
    Bulle = Bulle - 1;
    bulleTemp = bulleTemp + 28;  // Incrémente à la positio suivante en X
  }
  myGLCD.setColor(Couleur);  // Rétabli la couleur d'affichage d'origine
}

//Routine d'interruption ( Activée suite à la réception des impulsions reçue du compteur)
//*****************************************************************************************

void PulseCompteur() {
  ++NombrePulse;  // Incrémente le compteur d'impulsion du compteur de vitesse.  }
}

// routine de la fonction Timer pour la lecture de la vitesse.
//*********************************************************************************
ISR(TIMER1_COMPA_vect) {
  Speed = NombrePulse;
  NombrePulse = 0;
  SpeedOk = true;  // Valide et autorise l'affichage de la vitesse.
}

//************************************************************
// Lecture et Affichage de la vitesse.
//************************************************************
void Vitesse() {
  if (Veille == true) { return; }  // Mode veille activé, la moto ne roule pas, pas d'affichage de la vitesse.


  // Calcul du totalisateur Km journalier. 1 pulse = 0,16m
  //*********************************************************
  //Serial.print ( " Speed = ");
  //Serial.println (Speed );
  if (Speed == 0 && EepromTrue == false)  // Si pas de vitesse de déplacement et total Km pas encore sauvé.
  {

    EepromTrue = true;  // Drapeau pour ne pas écrire en boucle dans l'eeprom.
  }


  TotalImpulsion = TotalImpulsion + Speed;
  //Serial.print ( " TotalImpulsion = ");
  //Serial.println (TotalImpulsion );

  if (TotalImpulsion >= 625.00)  // valeur approximative pour 100m de déplacement. A optimiser aussi.
  {
    TotalJournalier = TotalJournalier + 0.10;  // ajoute 100m au compteur journalier
    TotalCompteur = TotalCompteur + 0.10;      // ajoute 100m au compteur total kilomètre.
    TotalImpulsion = TotalImpulsion - 625.00;  // soustrait la valeur calculée.
    eeprom_write_float(0, TotalJournalier);    // Sauve la valeur du compteur journalier en mémoire eeprom à l'adresse 0x00.
    eeprom_write_float(4, TotalCompteur);      // Sauve la valeur du compteur journalier en mémoire eeprom à l'adresse 0x04.
    // eeprom_write_float( 8,Speed);             // Sauve la valeur du compteur impulsion vitesse en mémoire eeprom à l'adresse 0x08.
  }

  if (TotalJournalier >= 9999.00)  // Au cas ou on laisse le compteur journalier dépasser sa capacité.
  {
    // Serial.println ( " TotalJournalier remis a zero suite à dépassement capaciter compteur ");
    TotalJournalier = 0, 00;
    eeprom_write_float(0, TotalJournalier);  // Sauve la valeur du compteur journalier en mémoire eeprom à l'adresse 0x00.
  }

  AffKm();  // Affichage du totalisateur Km journalier.

  //*****************************************************************************************************************

  Speed = (Speed * 1.3);  // Coefficient d'ajustement de la valeur.

  myGLCD.setFont(SevenSeg_XXXL_Num);             // 64x100 pixels
  word Couleur = myGLCD.getColor();              // Mémorise la couleur d'affichage en cours.
  myGLCD.setColor(CouleurCompteur);              // Couleur compteur pour le mode jour.
  if (Nuit == true) myGLCD.setColor(VGA_WHITE);  // Si le mode nuit est en cours, affichage des chiffres en blanc.
  if (Speed > 121) myGLCD.setColor(255, 40, 0);  // Si la vitesse est supérieur à 121Kmh,mais inférieur à 124Kmh, couleur d'affichage = orange.
  if (Speed > 124) myGLCD.setColor(VGA_RED);     // Si la vitesse dépasse 124Kmh, couleur d'affichage = rouge.

  myGLCD.printNumI(Speed, 25, 180, 3, '0');  // Affichage de la vitesse de déplacement à la positio X,Y, 3 chiffres,0 pour les chiffres manquants rep.(O)
  SpeedOk = false;
  myGLCD.setColor(Couleur);  //Rétablit la couleur d'affichage initiale des caractères .
}

void AffKm() {

  // Affichage du totalisateur Km journalier.
  //********************************************
  myGLCD.setFont(Arial_round_16x24);  //font

  myGLCD.printNumF(TotalJournalier, 1, 170, 298, ',', 6, '0');  // Affichage du totalisateur journalier à la positio X,Y, 6 chiffres,0 pour les chiffres manquants.rep.(w2)
}



//**********************************************************************************************************************
void Batterie()  // Lecture et Affichage de la tension de la batterie.


{
  myGLCD.setFont(SevenSegNumFontPlusPlus);
  float sensorBat = 0.0;
  sensorBat = analogRead(A1);         // Lecture de la valeur de la tension reçue sur l'entrée analogique A1.
  Bat = (sensorBat * 0.031826418);    // Correction pour convertire la tension reçue(0-5v) vers la tension réelle.
  myGLCD.printNumF(Bat, 1, 20, 360);  // Affichage de la tension de charge à la positio X,Y
}
//**********************************************************************************************************************
void ReadDS3231()  // Lecture et affichage de la date,heure.

{
  //second= Clock.getSecond();
  minutes = Clock.getMinute();
  heure = Clock.getHour(h12, PM);
  date = Clock.getDate();
  month = Clock.getMonth(Century);
  annee = Clock.getYear();
  myGLCD.setFont(BigFont);  //font 16X16

  xDate = 0;
  yDate = 0;

  //**************************************************************************************************
  // Affichage  de la date et de l'heure.
  //***********************************************************
  afficheDate(xDate, yDate);
  afficheHeure(xHeure, yHeure);
}

//************************************************************
// Affichage de la date
//**************************************
void afficheDate(int xDate, int yDate)

{
  myGLCD.setFont(BigFont);  // Font 16X16


  //if ( date < 10){ xDate= xDate+16; }

  myGLCD.printNumI(date, (xDate + 26), (yDate + 55), 2, '0');  // Affichage du jour: X,Y,2 chiffres, les vides remplacé par le chiffre 0
                                                               // if ( date < 10){ xDate= xDate - 16; }
  myGLCD.print("-", (xDate + 58), (yDate + 55));               // Affichage du séparateur

  myGLCD.printNumI(month, (xDate + 74), (yDate + 55), 2, '0');  // Affichage du mois

  myGLCD.print("-", (xDate + 106), (yDate + 55));  // Affichage du séparateur

  myGLCD.print("20", (xDate + 122), (yDate + 55));
  ;
  myGLCD.printNumI(annee, (xDate + 154), (yDate + 55));  // Affichage de l'année
}


//**************************************************
// Affichage de l'heure
//************************************

void afficheHeure(int xHeure, int yHeure) {

  myGLCD.setFont(SevenSegNumFontPlusPlus);                         // Font 32X50
  myGLCD.printNumI(heure, (xHeure + 20), (yHeure + 100), 2, '0');  // Affichage de l'heure X,Y,2 chiffres, les vides remplacé par le chiffre 0

  myGLCD.fillCircle((xHeure + 95), (yHeure + 115), 4);  // Point séparateur supérieur heure/min.
  myGLCD.fillCircle((xHeure + 95), (yHeure + 135), 4);  // Point séparateur inférieur heure/min.

  myGLCD.printNumI(minutes, (xHeure + 104), (yHeure + 100), 2, '0');  // Affichage des minutes X,Y,2 chiffres, les vides remplacé par le chiffre 0

  //myGLCD.fillCircle( (xHeure+160),(yHeure+150),2); // Point séparateur min./secondes
  //myGLCD.fillCircle( (xHeure+160),(yHeure+160),2); // Point séparateur min./secondes

  //myGLCD.setFont(BigFont); // font 16X16
  //myGLCD.printNumI(second, (xHeure+170), (yHeure+145), 2,'0'); // Affichage des secondes X,Y,2 chiffres, les vides remplacé par le chiffre 0
}



//**********************************************************************************************************************
void Pression()  // Lecture et affichage de la tension de la pression de l'amortisseur arrière.
{
  myGLCD.setFont(Arial_round_16x24);  //font
  float sensorPression = 0.0;

  sensorPression = analogRead(A4);  // Lecture de la valeur sur l'entrée analogique A4

  /*Serial.print(" Pression = "); 
   Serial.println (sensorPression);
   Serial.println("");*/

  float Pression = ((sensorPression * 0.003067485) - 0.4);  //  v*(5.0/1023.0)

  //myGLCD.setColor(0, 255, 0); // couleur d'affichage = vert
  //Pression = 10;
  myGLCD.printNumF(Pression, 0, 70, 298);  // Affichage de la pression amortisseur à la positio X,Y
}

//**********************************************************************************************************************
void Gear()  // Lecture et affichage de la vitesse boite sélectionnée.
{

  boolean VSelect = false;

  if (digitalRead(GearN) == HIGH) VitSelect = "0", VSelect = true;  // Fil Bleu.
  if (digitalRead(Gear2) == HIGH) VitSelect = "2", VSelect = true;  // Fil Violet.
  if (digitalRead(Gear3) == HIGH) VitSelect = "3", VSelect = true;  // Fil Gris.
  if (digitalRead(Gear4) == HIGH) VitSelect = "4", VSelect = true;  // Fil Blanc.
  if (digitalRead(Gear5) == HIGH) VitSelect = "5", VSelect = true;  // Fil Noir.

  myGLCD.setFont(GroteskBold32x64);  // Font 32X50

  myGLCD.print((""), 260, 220);  // Effacement de la vitesse sélectionnée à la positio X,Y.

  if (VSelect == false) VitSelect = "1";  // Si pas,N,2,3,4 ou 5 c'est la première vitesse qui est enclenchée.


  //VitSelect= "3";

  if (VitSelect == VitSel) { return; }  // Pour empécher le scintillement de l'affichage.


  word BackC = myGLCD.getBackColor();

  myGLCD.setColor(VGA_LIME);        // ecriture en vert.
  myGLCD.setBackColor(VGA_LIME);    // Fond en vert
  myGLCD.fillCircle(270, 240, 40);  // Cercle du fond en noir
  myGLCD.setColor(VGA_BLACK);       // ecriture en noir

  if (Nuit == false)  // Si mode jour.
  {
    myGLCD.drawCircle(270, 240, 41);  // Entourage d'un cercle noir.
  }

  myGLCD.print((VitSelect), 255, 210);  // Affichage de la vitesse sélectionnée à la positio X,Y.(rep.M
  myGLCD.setColor(CouleurAff);          // Couleur d'affichage normal.
  myGLCD.setBackColor(BackC);


  VitSel = VitSelect;
}

//**********************************************************************************************************************
void HorlogeVeille()  // Lecture et affichage de la date, heure et température lors du contact sur ACC.

{
  /* eeprom_write_float( 0,TotalJournalier);   // Sauve la valeur du compteur journalier en mémoire eeprom à l'adresse 0x00. 
  eeprom_write_float( 4,TotalCompteur);     // Sauve la valeur du compteur journalier en mémoire eeprom à l'adresse 0x04.
  eeprom_write_float( 8,Speed);             // Sauve la valeur du compteur impulsion vitesse en mémoire eeprom à l'adresse 0x08.
  */

  //EepromTrue = false; // active le drapeau pour enregistrement dans la mémoire eeprom.
  //second=Clock.getSecond();
  minutes = Clock.getMinute();      // Lecture des minutes.
  heure = Clock.getHour(h12, PM);   // Lecture de l'heure.
  date = Clock.getDate();           // Lecture de la date.
  month = Clock.getMonth(Century);  // Lecture du mois.
  annee = Clock.getYear();          // Lecture de l'année.

  myGLCD.setFont(BigFont);     // Font 16X16
  myGLCD.setColor(0, 0, 255);  // Bleu foncé
  myGLCD.print("*****************", 24, 40);
  myGLCD.setColor(0, 5, 255);  // Bleu clair
  myGLCD.print("*", 24, 60);
  myGLCD.setColor(0, 10, 255);  // Bleu clair
  myGLCD.print("*", 280, 60);

  myGLCD.setColor(255, 255, 0);  // Jaune
  myGLCD.print("  HONDA     ", 88, 60);

  myGLCD.setColor(0, 20, 255);  // Bleu clair
  myGLCD.print("*", 24, 80);
  myGLCD.setColor(0, 25, 255);  // Bleu clair
  myGLCD.print("*", 280, 80);
  myGLCD.setColor(0, 30, 255);  // Bleu clair
  myGLCD.print("*", 24, 100);
  myGLCD.setColor(0, 35, 255);  // Bleu clair
  myGLCD.print("*", 280, 100);

  myGLCD.setColor(255, 255, 0);  // Jaune
  myGLCD.print("GOLDWING 1500", 56, 84);
  myGLCD.print("( 1988 )", 84, 104);

  // Entretien = TotalCompteur+5000;
  myGLCD.print("* ENTRETIEN * ", 60, 136);
  myGLCD.print("Dans:", 66, 160);

  TotalEntretien = EntretienHuile - TotalCompteur;
  if (TotalEntretien <= 0) {
    TotalEntretien = 0;
  }

  myGLCD.printNumI(TotalEntretien, 146, 160);  // Affichage du totalisateur Km à la positio X,Y, 5 chiffres
  myGLCD.print("Km", 210, 160);


  // myGLCD.setFont(Arial_round_16x24);
  myGLCD.print("Tot: ", 50, 200);
  myGLCD.printNumF(TotalCompteur, 1, 115, 200, ',', 8, '0');  // Affichage du totalisateur Km à la positio X,Y, 6 chiffres,0 pour les chiffres manquants.
  myGLCD.print("Km", 242, 200);


  myGLCD.setFont(BigFont);  // Font 16X16

  myGLCD.setColor(0, 40, 255);  // Bleu clair
  myGLCD.print("*****************", 24, 120);
  myGLCD.print("*", 24, 140);
  myGLCD.print("*", 280, 140);
  myGLCD.print("*", 24, 160);
  myGLCD.print("*", 280, 160);
  myGLCD.print("*", 24, 160);
  myGLCD.print("*", 280, 160);
  myGLCD.print("*****************", 24, 180);

  myGLCD.print("*", 24, 200);
  myGLCD.print("*", 280, 200);
  myGLCD.print("*****************", 24, 220);
  myGLCD.print("*", 24, 240);
  myGLCD.print("*", 280, 240);
  myGLCD.print("*", 24, 260);
  myGLCD.print("*", 280, 260);
  myGLCD.print("*", 24, 280);
  myGLCD.print("*", 280, 280);
  myGLCD.print("*", 24, 300);
  myGLCD.print("*", 280, 300);
  myGLCD.print("*", 24, 320);
  myGLCD.print("*", 280, 320);


  myGLCD.print("*****************", 24, 360);
  myGLCD.print("*", 24, 340);
  myGLCD.print("*", 280, 340);
  myGLCD.print("*", 24, 380);
  myGLCD.print("*", 280, 380);

  myGLCD.print("*", 24, 400);
  myGLCD.print("*", 280, 400);
  myGLCD.print("*", 24, 400);
  myGLCD.print("*", 280, 400);
  myGLCD.print("*", 24, 420);
  myGLCD.print("*", 280, 420);
  myGLCD.print("*****************", 24, 440);
  myGLCD.setColor(VGA_WHITE);  // Couleur d'affichage = blanc
  myGLCD.print("(jpb@jpbrabant.be)", 20, 460);


  myGLCD.setColor(0, 255, 0);  // Vert
  afficheDate(50, 190);
  myGLCD.print("V.", 204, 340);         //
  myGLCD.printNumI(Version, 240, 340);  //

  afficheHeure(40, 180);
  myGLCD.setColor(VGA_AQUA);  // Bleu foncé

  afficheTemp(20, 90);  // Affichage de la température extérieure.
  //Radiateur();  // Gestion des ventilateurs de refroidissement du radiateur.
}



//*********************************************************************************
// Lecture et Affichage de la température extérieure.
//******************************************************

void afficheTemp(int xTemp, int yTemp)

{
  myGLCD.setFont(Grotesk24x48);

  Temp();                                                         // Lecture de la température
  Temperature = Temperature - 3;                                  //
                                                                  //Serial.println ( Temperature );
  myGLCD.printNumF(Temperature, 1, (xTemp + 90), (yTemp + 300));  // Affichage de la température extérieur à la positio X,Y
  myGLCD.drawCircle((xTemp + 202), (yTemp + 310), 4);             // symbole degrés
}

//****************************************************
// Sous-routine pour la lecture du thermometre DHT22 *
//****************************************************
void Temp()  // Lecture de la température ambiante
{

  sensors_event_t event;
  // dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    //Serial.println("Error reading temperature!");
  } else {
    Temperature = event.temperature;
  }
}
//**********************************************************************************************
void Clignote()  // Gestion de la temporisation pour le clignotement.
//**********************************************************************************************
{
  Temps = millis();

  if (Temps - TempsPasser >= TempClignote)  // Si le temps passé arrivé à la valeur,
  {
    // Sauve le temps pour le calul du prochain passage.
    TempsPasser = Temps;

    // Si c'est allumé on éteind.
    if (Allume == true)  //
    {
      Allume = false;

    } else  // Si pas allumé, on allume.
    {
      Allume = true;
    }
  }
}
//*********************************************************************************
// Affichage de la fonction debug.
//******************************************************
void WriteStatut() {
  myGLCD.setBackColor(VGA_BLACK);
  myGLCD.clrScr();
  myGLCD.setColor(VGA_WHITE);
  myGLCD.setFont(BigFont);  // Font 16X16

  myGLCD.print(" Mode status", CENTER, 0);
  myGLCD.print(" *************** ", CENTER, 20);

  for (int a = 1; a == 1; a = 1)  // boucle infinie
  {
    myGLCD.print(" ContactOn  = ", 0, 40);
    myGLCD.printNumI(digitalRead(MotoOn), 224, 40);

    myGLCD.print(" Essence    = ", 0, 60);
    myGLCD.printNumI(analogRead(A2), 224, 60);


    myGLCD.print(" TempMoteur = ", 0, 80);
    myGLCD.printNumI(analogRead(A0), 224, 80);

    myGLCD.print(" LDR        = ", 0, 100);
    int Ld = analogRead(PhotoRes);
    myGLCD.printNumI(Ld, 224, 100);

    myGLCD.print(" Compteur   = ", 0, 120);
    myGLCD.printNumI(digitalRead(2), 224, 120);

    myGLCD.print(" Pression K = ", 0, 140);
    myGLCD.printNumI(analogRead(A4), 224, 140);

    myGLCD.print(" Regl.Minute= ", 0, 160);
    myGLCD.printNumI(digitalRead(6), 224, 160);

    myGLCD.print(" Regl.Heure = ", 0, 180);
    myGLCD.printNumI(digitalRead(5), 224, 180);


    myGLCD.print(" Bequille   = ", 0, 200);
    myGLCD.printNumI(digitalRead(8), 224, 200);

    myGLCD.print(" Gear N     = ", 0, 220);
    myGLCD.printNumI(digitalRead(9), 224, 220);

    myGLCD.print(" Gear 2     = ", 0, 240);
    myGLCD.printNumI(digitalRead(10), 224, 240);

    myGLCD.print(" Gear 3     = ", 0, 260);
    myGLCD.printNumI(digitalRead(11), 224, 260);

    myGLCD.print(" Gear 4     = ", 0, 280);
    myGLCD.printNumI(digitalRead(12), 224, 280);

    myGLCD.print(" Gear 5     = ", 0, 300);
    myGLCD.printNumI(digitalRead(13), 224, 300);

    myGLCD.print(" TensionBat = ", 0, 320);
    myGLCD.printNumI(analogRead(A1), 224, 320);

    myGLCD.print(" Heure      = ", 0, 340);
    myGLCD.printNumI(Clock.getHour(h12, PM), 224, 340);

    myGLCD.print(" Minute     = ", 0, 360);
    myGLCD.printNumI(Clock.getMinute(), 224, 360);

    myGLCD.print(" Jour       = ", 0, 380);
    myGLCD.printNumI(Clock.getDate(), 224, 380);

    myGLCD.print(" Mois       = ", 0, 400);
    myGLCD.printNumI(Clock.getMonth(Century), 224, 400);

    myGLCD.print(" Annee      = ", 0, 420);
    myGLCD.printNumI(Clock.getYear(), 224, 420);

    myGLCD.print(" TempExt.   = ", 0, 440);
    Temp();  // Lecture de la température
    myGLCD.printNumF(SensorTemp, 1, 224, 440);

    myGLCD.print(" M. Arriere = ", 0, 460);
    myGLCD.printNumI(digitalRead(A6), 224, 460);
  }
}

void Radio_lecture() {
  myGLCD.setFont(Arial_round_16x24);  //

  switch (Code) {


    //*****************************************************
    case 1:  // 1=K7 0=Radio


      ClearRadio();  // Effacement de l'espace radio/K7

      break;


    //********************************************************
    case 2:  // 0= Flèche vers la droite (K7)

      if (Data == 0) {
        myGLCD.print("    ", 210, 95);  // Effacement du sens de la K7 à la positio X,Y.
        myGLCD.print("--->", 210, 95);  // Affichage du sens de la K7 à la position X,Y.
      }
      break;
    //*********************************************************
    case 3:  // 0= Flèche vers la gauche (K7)
      if (Data == 0) {
        myGLCD.print("    ", 210, 95);  // Effacement du sens de la K7 à la positio X,Y.
        myGLCD.print("<---", 210, 95);  // Affichage du sens de la K7 à la position X,Y.
      }
      break;
    //*********************************************************
    case 4:  // 0=DOLBY

      if (Mute == true) { break; }
      myGLCD.setFont(Retro8x16);
      if (Data == 0) {
        myGLCD.print("Dolby", 195, 145);  // Affiche l'indiquateur Dolby à la position X,Y.
      }
      if (Data == 1) {
        myGLCD.print("     ", 195, 145);  // Efface l'indiquateur Dolby à la position X,Y.
      }
      break;
    //**********************************************************
    case 5:  // 0=AMB

      myGLCD.setFont(Retro8x16);
      if (Mute == true) { break; }
      if (Data == 0) {
        myGLCD.print("AMB ", 195, 145);  // Affiche l'indiquateur AMB à la position X,Y.
      }
      if (Data == 1) {
        myGLCD.print("    ", 195, 145);  // Efface l'indiquateur AMB à la position X,Y.
      }
      break;
    //***********************************************************
    case 6:  // 0=Scan

      if (Data == 0) {
        myGLCD.setFont(Retro8x16);
        myGLCD.print("SCAN", 245, 145);  // Affiche l'indiquateur EAR à la position X,Y.
      }
      if (Data == 1) {
        myGLCD.setFont(Retro8x16);
        myGLCD.print("    ", 245, 145);  // Affiche l'indiquateur HP à la position X,Y.
      }

      break;
    //***********************************************************
    case 7:  // Casque/HP

      if (Data == 0) {
        myGLCD.setFont(Retro8x16);
        myGLCD.print("Ear", 285, 145);  // Affiche l'indiquateur EAR à la position X,Y.
      }
      if (Data == 1) {
        myGLCD.setFont(Retro8x16);
        myGLCD.print(" HP", 285, 145);  // Affiche l'indiquateur HP à la position X,Y.
      }
      break;
    //************************************************************
    case 8:  // 0=Mute

      if (Data == 0) {
        Mute = true;
        // Clignote();
        // if ( Allume == true )

        myGLCD.setFont(Retro8x16);
        myGLCD.print("Mute ", 195, 145);  // Affiche l'indiquateur Mute
      }
      if (Data == 1) {
        myGLCD.print("    ", 195, 145);  // Efface l'indiquateur Mute
        Mute = false;
      }
      break;
    //*************************************************************
    case 9:  // 0= ligne de séparation

      if (Data == 0) {
        myGLCD.drawLine(190, 135, 315, 135);  // Ligne K8
      }
      if (Data == 1) {
        CouleurAff = myGLCD.getColor();       // Mémorise la couleur d'affichage des caractères.
        myGLCD.setColor(0, 0, 0);             // Initialise la couleur de l'écriture des caractères.
        myGLCD.drawLine(190, 135, 315, 135);  // Ligne K8
        myGLCD.setColor(CouleurAff);          // Initialise la couleur de l'écriture des caractères.
      }
      break;
    //***********************************************************
    case 10:  // 0= STéréo
      myGLCD.setFont(Retro8x16);
      if (Data == 0) {
        myGLCD.setFont(Retro8x16);
        myGLCD.print("ST", 293, 110);  // Affiche l'indiquateur du mode stéréo à la position X,Y.
      }
      if (Data == 1) {
        myGLCD.print("ST", 293, 110);       // Efface l'indiquateur du mode stéréo à la position X,Y.
        myGLCD.drawLine(190, 75, 315, 75);  // Ligne K1 Séparation supérieure radio
      }
      break;
    //************************************************************
    case 11:  // 1= M clignotant

      if (Data == 0) {
        myGLCD.print("M", 275, 50);  // Affichage du symbole "M" à la position X,Y.
      }
      if (Data == 1) {
        myGLCD.print(" ", 275, 50);  // Efface le symbole "M" à la position X,Y.
      }
      break;
    //*************************************************************
    case 12:  // 0=AM 1=FM

      Mode_Radio = Data;    // Mémorise: 0=AM ou 1=FM
      if (Mode_Radio == 1)  // 1=FM
      {
        myGLCD.print("FM", 195, 50);        // K3 - Affichage du mode FM à la position X,Y.
        myGLCD.setFont(Retro8x16);          //font 8X16
        myGLCD.print("Mhz", 285, 82);       // K11- Affichage Type de fréquence à la position X,Y.
        myGLCD.setFont(Arial_round_16x24);  //font
      }

      if (Mode_Radio == 0)  //  0=AM
      {
        myGLCD.print("AM", 195, 50);        // K3 -  Affichage du mode FM à la position X,Y.
        myGLCD.setFont(Retro8x16);          //font 8X16
        myGLCD.print("Khz", 285, 82);       // K11- Affichage duType de fréquence à la position X,Y.
        myGLCD.setFont(Arial_round_16x24);  //font
      }
      break;
    //*************************************************************
    case 13:  // Numéro de la mémoire active

      if (Data == 127) {
        myGLCD.print("_", 295, 50);  //
      } else {
        myGLCD.print("M ", 275, 50);      // Affichage du symbole "M" à la position X,Y.
        myGLCD.printNumI(Data, 295, 50);  //
      }
      break;
    //*************************************************************
    case 14:  // code Fréquence

      myGLCD.setFont(Grotesk16x32);  //font

      int Frequence = Data << 8;
      Frequence = Frequence + Frequ;
      Sfrequence = String(Frequence);  // Conversion int to String

      if (Mode_Radio == 1)  // si FM
      {
        if (Sfrequence.length() == 4) {
          Sfrequence = Sfrequence.substring(0, 3) + '.' + Sfrequence.substring(3);  // intercale le "." entre la 3iem position et la 4
        }

        if (Sfrequence.length() == 3)  // Si la fréquence en dessous de 100
        {
          Sfrequence = Sfrequence.substring(0, 2) + '.' + Sfrequence.substring(2);  //  intercale le "." entre la 2iem position et la 3

          Sfrequence = Sfrequence + " ";  // intercale le "." entre la 2iem position et la 3
        }
      }

      if (Mode_Radio == 0)  // si AM
      {
        int a = Sfrequence.length();
        if (a == 3) {
          Sfrequence = Sfrequence + "  ";
        }
        if (a == 4) {
          Sfrequence = Sfrequence + " ";
        }
      }


      myGLCD.print(Sfrequence, 205, 90);  //

      break;

      //*************************************************************
    default:

      break;
      //*************************************************************
  }
}

void ClearRadio() {
  myGLCD.setColor(CouleurFond);
  myGLCD.fillRect(191, 50, 314, 169);  // Efface la zone d'affichage de la radio/k7
  myGLCD.setColor(CouleurAff);         // Rétablit la couleur d'affichage des caractères

  myGLCD.drawLine(190, 75, 315, 75);    // Ligne K1 Séparation supérieure radio
  myGLCD.drawLine(190, 135, 315, 135);  // Ligne K8 Séparation inférieure radio
  myGLCD.setFont(Arial_round_16x24);    //font

  if (Data == 0)  // Si c'est mode RADIO
  {
    myGLCD.print("FM", 195, 50);  // K3 - Affichage du mode FM à la position X,Y.
    myGLCD.print("M_", 275, 50);  // Affichage du symbole "M" à la position X,Y.
  }
  if (Data == 1)  // Si c'est mode K7
  {
    myGLCD.print("K7", 240, 50);  // Affichage du mode K7 à la position X,Y.
  }
}

// Fin des routines.
//*********************************************************************************************************************************************

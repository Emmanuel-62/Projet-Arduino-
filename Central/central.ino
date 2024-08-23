/* CENTRAL

  * Central est une boite qui regroupe plusieurs capteurs dans le but de collecter des données, de les stockers et d'envoyer des données sur un serveur.
  * Les données sont collectés avec des capteurs, stockées dans une carte SD et envoyées avec un module GSM SIM800L V2.
  * Module Carte SD : stocke les données sur un fichier CSV ou TXT (Pin GND(Noir) : GND; Pin 5V(Rouge) : 5V; Pin SS(#) : 53; Pin SCK(#) : 52; Pin MOSI(#) : 51; Pin MISO(#) : 50)
  * Capteur SHT 35 : pour la température et l'humidité (Pin GND(Noir) : GND; Pin 5V(Rouge) : 5V; Pin SDA(Blanc) : 20; Pin SCL(Jaune) : 21)
  * Capteur Next PM : pour les particules d'air
  
*/

#include "Seeed_SHT35.h"
#include "NextPM.h"
#include "SD.h"

char nomDuFichier [] = "datas.csv";
File monFichier;

// lecteur de la carte SD branché sur la broche 10
#define carteSelect 53

// Définition des broches I2C
#define SDA_PIN 20 // Broche SDA pour la communication I2C
#define SCL_PIN 21 // Broche SCL pour la communication I2C

// Définir les broches du moteur et des LED
#define MOTOR_PIN   49 // Broche pour le moteur d'évacuation
#define LedVerte    48
#define LedRouge    47
#define LedJaune    46
#define LedBlanche  45
#define LedBleu     44

// Création des instances 
SHT35 sensor(SCL_PIN); // Création d'une instance du capteur SHT35 avec les broches I2C
NextPM capteur(Serial2); // Déclaration de la variable capteur dans la portée globale

int compteurLectures = 0; // Compteur de lectures de capteur
unsigned long dernierPassage = 0; // Temps du dernier passage
bool moteurAllume = false; // État du moteur
unsigned long tempsDebutMoteur = 0; // Temps de début de l'allumage du moteur

unsigned long tempsDebutAttente = 0; // Temps de début de l'attente après l'arrêt du moteur
const unsigned long tempsAttente = 60000; // Attente de 10 secondes entre les cycles du moteur

void setup() {
  // Initialiser la communication série
  Serial.begin(9600);
  Serial.println("Démarrage du système...");
  Serial.println("Initialisation des modules et des capteurs...");

  // Initialisation du module Carte SD
  init_cartesd();

  // Creation du Ficgier d'enregistrement 
  creat_fichier();

  // Appelle à la fonction initialiserCapteur pour initialiser le capteur et afficher le message approprié
  if (!initSht35()) {
    Serial.println("Échec de l'initialisation du capteur SHT35. Arrêt du programme.");
    while (1); // Arrêt du programme si l'initialisation échoue
  }

  Serial.print("Initialisation du capteur Next PM =====> ");
  if (!initNextPM()) { // Vérifie si l'initialisation a réussi
    Serial.println("Échec de l'initialisation du capteur Next PM. Arrêt du programme.");
    while (1); // Boucle infinie si l'initialisation échoue
  }

  // Initialiser la broche du moteur comme une sortie
  pinMode(MOTOR_PIN, OUTPUT);
  
  // Initialiser les broches des LED comme des sorties
  pinMode(LedVerte, OUTPUT);
  pinMode(LedRouge, OUTPUT);
  pinMode(LedJaune, OUTPUT);
  pinMode(LedBlanche, OUTPUT);
  pinMode(LedBleu, OUTPUT);

  // Vérification du fonctionnement du moteur et des LED pendant 5 secondes
  Serial.println("Vérification du fonctionnement du moteur et des LED...");
  digitalWrite(MOTOR_PIN, HIGH);
  digitalWrite(LedVerte, HIGH);
  digitalWrite(LedRouge, HIGH);
  digitalWrite(LedJaune, HIGH);
  digitalWrite(LedBlanche, HIGH);
  digitalWrite(LedBleu, HIGH);
  delay(5000);
  digitalWrite(MOTOR_PIN, LOW);
  digitalWrite(LedVerte, LOW);
  digitalWrite(LedRouge, LOW);
  digitalWrite(LedJaune, LOW);
  digitalWrite(LedBlanche, LOW);
  digitalWrite(LedBleu, LOW);

  Serial.println("Vérification terminée.");

  delay(1000); // Attente après l'initialisation
  Serial.println("Système prêt.");
}

void loop() {
  // Vérifier si le moteur doit être éteint après 1 minute de fonctionnement
  if (moteurAllume && (millis() - tempsDebutMoteur >= 60000)) {
    digitalWrite(MOTOR_PIN, LOW);
    digitalWrite(LedRouge, LOW);
    moteurAllume = false;
    tempsDebutAttente = millis(); // Commencer à compter l'attente après l'arrêt du moteur
    Serial.println("Moteur et LED rouge éteints.");
  }

  // Lire les données des capteurs chaque seconde
  if (millis() - dernierPassage >= 1000) {
    dernierPassage = millis();
    compteurLectures++; // Incrémenter le compteur de lectures
    
    // Allumer la LED verte pour indiquer que les capteurs sont en cours de lecture
    digitalWrite(LedVerte, HIGH);
    String DataCap = envoiDataCap();
    digitalWrite(LedVerte, LOW); // Éteindre la LED verte après la lecture des données

    // Afficher les données collectées
    Serial.println("==============================================================================================================");
    Serial.print("Lecture numéro : ");
    Serial.println(compteurLectures);
    Serial.println(DataCap);
    enreg_cartesd(DataCap);

    // Si 60 passages de données de capteurs ont eu lieu, allumer le moteur et la LED rouge
    if (compteurLectures >= 60 && !moteurAllume && (millis() - tempsDebutAttente >= tempsAttente)) {
      digitalWrite(MOTOR_PIN, HIGH);
      digitalWrite(LedRouge, HIGH);
      moteurAllume = true;
      tempsDebutMoteur = millis(); // Enregistrer le temps d'allumage du moteur
      compteurLectures = 0; // Réinitialiser le compteur de lectures
      Serial.println("Moteur et LED rouge allumés.");
    }
  }
}

//******************************************************** SHT 35 ************************************************************
bool initSht35() {
  Serial.print("Initialisation du SHT 35 ======> ");
  if (!sensor.init()) {
    Serial.println("OK SHT 35"); // Envoie "OK" si l'initialisation réussit
    return true; // Retourne true pour indiquer que l'initialisation a réussi
  } else {
    Serial.println("Échec SHT 35"); // Envoie "Échec" si l'initialisation échoue
    return false; // Retourne false pour indiquer que l'initialisation a échoué
  }
}

void AfficherDonneesSht35() {
  float temperature, humidite;
  lireDonneesSht35(&temperature, &humidite);

  Serial.print("Température : ");
  Serial.print(temperature);
  Serial.println("°C");
  Serial.print("Humidité : ");
  Serial.print(humidite);
  Serial.println("%");
}

// Fonction pour lire les données du capteur
void lireDonneesSht35(float* temperature, float* humidite) {
  sensor.read_meas_data_single_shot(HIGH_REP_WITH_STRCH, temperature, humidite);
}

//********************************************************* Next PM *****************************************************************
// Fonction pour initialiser le capteur PM
bool initNextPM() {
  // Appel de la méthode configure()
  capteur.configure();

  // Tente de lire des données du capteur pour confirmer l'initialisation
  PM_DATA donnees;
  Serial.print("Initialisation du Next PM ======> ");
  if (capteur.read_1min(donnees)) {
    Serial.println("OK Next PM");
    return true; // Initialisation réussie
  } else {
    Serial.println("Échec Next PM");
    return false; // Initialisation échouée
  }
}

PM_DATA lireDonneesNextPM() {
  PM_DATA donnees;
  if (capteur.read_1min(donnees)) {
    return donnees;
  } else {
    Serial.println("Échec lecture capteur Next PM");
    return donnees; // Retourne une structure vide ou initialisée selon votre logique
  }
}

void afficherDonnees(PM_DATA donnees) {
  float pm1 = donnees.PM_UG_1_0;
  float pm10 = donnees.PM_UG_10_0;
  float pm25 = donnees.PM_UG_2_5;
  Serial.print("Données:");
  Serial.print(" PM 1.0 (ug/m3): ");
  Serial.print(pm1);
  Serial.print(" -- PM 2.5 (ug/m3): ");
  Serial.print(pm25);
  Serial.print(" -- PM 10.0 (ug/m3): ");
  Serial.println(pm10);
}

//********************************************************* DONNEES *****************************************************************

String envoiDataCap() {
  // Initialisation des variables pour stocker les données du capteur
  float temperature, humidite;
  float pm1, pm10, pm25;

  // Lecture des données du capteur SHT35
  lireDonneesSht35(&temperature, &humidite);

  // Lecture des données du capteur Next PM
  PM_DATA donnees = lireDonneesNextPM();

  // Extraction des valeurs PM
  pm1 = donnees.PM_UG_1_0;
  pm10 = donnees.PM_UG_10_0;
  pm25 = donnees.PM_UG_2_5;

  // Construction de la chaîne JSON avec les données collectées
  String DataCap = "";

  // Construction de chaque paire nom de champ - valeur
  DataCap += "Température : " + String(temperature) + " C\n";
  DataCap += "Humidité : " + String(humidite) + " %\n";
  DataCap += "PM 1.0 (ug/m3) : " + String(pm1) + "\n";
  DataCap += "PM 2.5 (ug/m3) : " + String(pm25) + "\n";
  DataCap += "PM 10.0 (ug/m3) : " + String(pm10) + "\n";

  return DataCap;
}

//********************************************************* CARTE SD *****************************************************************
void init_cartesd() {
  Serial.println("Initialisation de la carte SD...");
  if (!SD.begin(carteSelect)) {
    Serial.println("Échec de l'initialisation de la carte SD !");
  } else {
    Serial.println("Initialisation de la carte SD réussie.");
    digitalWrite(LedJaune, HIGH);
    delay(500);
    digitalWrite(LedJaune, LOW);
  }
}

void creat_fichier() {
  Serial.print(F("Enregistrement d'une chaîne de caractères dans ce fichier ====> "));
  monFichier = SD.open(nomDuFichier, FILE_WRITE);
  if (monFichier) {
    Serial.print("OK Écriture d'une chaîne de caractères dans le fichier '");
    Serial.print(nomDuFichier);
    Serial.println("'");
    monFichier.println("L'enregistrement dans ce fichier a débuté à cette date : " __DATE__ " et l'heure " __TIME__); 
    monFichier.close();
  } else {
    Serial.print("-> Erreur lors de la tentative de création du fichier '");
    Serial.print(nomDuFichier);
    Serial.println("'");
  }
  Serial.println();
}

void enreg_cartesd(String DataCap) {
  Serial.println("Enregistrement des données sur la carte SD...");
  monFichier = SD.open(nomDuFichier, FILE_WRITE);
  if (monFichier) {
    if (monFichier.println(DataCap)) {
      Serial.println("Enregistrement réussi sur la carte SD.");
      digitalWrite(LedJaune, HIGH);
      delay(500);
      digitalWrite(LedJaune, LOW);
    } else {
      Serial.println("Erreur lors de l'écriture dans le fichier sur la carte SD !");
    }
    monFichier.close();
  } else {
    Serial.print("Erreur lors de l'ouverture du fichier sur la carte SD : ");
    Serial.println(nomDuFichier);
  }
}

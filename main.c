#include <Adafruit_NeoPixel.h>

const int numLeds = 150;
int pinoLed = 6;

Adafruit_NeoPixel fita = Adafruit_NeoPixel(numLeds, pinoLed, NEO_GRB + NEO_KHZ800);

/*
    A FITA É ASSIM, COMO matriz VISUAL 5x5 :

    (0,0) (0,1) (0,2) (0,3) (0,4) (0,5) (0,6)  =   0  1  2  3  4  5  6
    (1,0) (1,1) (1,2) (1,3) (1,4) (1,5) (1,6)  =  18 17 16 15 14 13 12
    (2,0) (2,1) (2,2) (2,3) (2,4) (2,5) (2,6)  =  24 25 26 27 28 29 30
    (3,0) (3,1) (3,2) (3,3) (3,4) (3,5) (3,6)  =  42 41 40 39 38 37 36
    (4,0) (4,1) (4,2) (4,3) (4,4) (4,5) (4,6)  =  48 49 50 51 52 53 54
    (5,0) (5,1) (5,2) (5,3) (5,4) (5,5) (5,6)  =  66 65 64 63 62 61 60
    (6,0) (6,1) (6,2) (6,3) (6,4) (6,5) (6,6)  =  72 73 74 75 76 77 78

    Laguinho fixo: (3,3) = índice 39.
*/

int idxLaguinho = 39;
bool piscarAtivo[numLeds] = { false };
unsigned long ultimaTroca = 0;
bool estadoPisca = false;

int posicaoParaIndice(int linha, int coluna) {
  int matriz[7][7] = {
    { 0, 1, 2, 3, 4, 5, 6 },
    { 18, 17, 16, 15, 14, 13, 12 },
    { 24, 25, 26, 27, 28, 29, 30 },
    { 42, 41, 40, 39, 38, 37, 36 },
    { 48, 49, 50, 51, 52, 53, 54 },
    { 66, 65, 64, 63, 62, 61, 60 },
    { 72, 73, 74, 75, 76, 77, 78 }
  };
  return matriz[linha][coluna];
}

void acender(int linha, int coluna) {
  int idx = posicaoParaIndice(linha, coluna);
  piscarAtivo[idx] = false;
  fita.setPixelColor(idx, fita.Color(255, 0, 0));
  fita.show();
  Serial.println("LED aceso.");
}

void setup() {
  Serial.begin(9600);
  pinMode(pinoLed, OUTPUT);
  fita.begin();
  fita.setPixelColor(idxLaguinho, fita.Color(0, 0, 255));  // azul
  fita.show();
}

void loop() {
  if (Serial.available()) {
    String entrada = Serial.readStringUntil('\n');
    entrada.trim();

    if (entrada.equalsIgnoreCase("clear")) {
      limparmatrizDeLEDs();
      Serial.println("Todos os LEDs foram apagados.");
      return;
    }

    int barra1 = entrada.indexOf('/');
    if (barra1 == -1) {
      Serial.println("Formato inválido. Use: acao/linha,coluna");
      return;
    }

    String acao = entrada.substring(0, barra1);
    String coordenadas = entrada.substring(barra1 + 1);

    int virgula = coordenadas.indexOf(',');  // podemos nao usar, pois sera sempre 1 linha e 1 coluna, logo 1 digito de linha e 1 digito de coluna.
    if (virgula == -1) {
      Serial.println("Coordenadas inválidas. Use linha,coluna");
      return;
    }

    int linha = coordenadas.substring(0, virgula).toInt();
    int coluna = coordenadas.substring(virgula + 1).toInt();

    if (linha < 0 || linha > 6 || coluna < 0 || coluna > 6) {
      Serial.println("Linha ou coluna fora dos limites.");
      return;
    }

    int idx = posicaoParaIndice(linha, coluna);

    if (idx == idxLaguinho) {
      Serial.println("LED protegido: laguinho");
      return;
    }

    if (acao == "acender") {
      acender(linha,coluna);
    } else if (acao == "apagar") {
      piscarAtivo[idx] = false;
      fita.setPixelColor(idx, 0);
      //fita.setPixelColor(idxLaguinho, fita.Color(0, 0, 255));   // inutil
      fita.show();
      Serial.println("LED apagado.");
    } else if (acao == "piscar") {
      piscarAtivo[idx] = true;
      Serial.println("LED piscando.");
    } else {
      Serial.println("Ação inválida. Use: acender, apagar ou piscar");
    }
  }

  if (millis() - ultimaTroca >= 300) {
    ultimaTroca = millis();
    estadoPisca = !estadoPisca;

    for (int i = 0; i < numLeds; i++) {
      if (piscarAtivo[i]) {
        if (estadoPisca == true) {
          fita.setPixelColor(i, fita.Color(0, 255, 0)); //  pisca verde 
        } else {
          fita.setPixelColor(i, 0);
        }
      }
    }
    fita.show();
  }
}

void limparmatrizDeLEDs() {
  for (int i = 0; i < numLeds; i++) {
    fita.setPixelColor(i, 0);
    piscarAtivo[i] = false;
  }
  fita.setPixelColor(idxLaguinho, fita.Color(0, 0, 255));
  fita.show();
}

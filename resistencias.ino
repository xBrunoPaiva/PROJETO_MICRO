#include <MuxShield.h>
#include <SoftwareSerial.h>
#define NUM_LINHAS 7
#define NUM_COLUNAS 7

MuxShield muxShield;
SoftwareSerial SerialSoft(31, 53);
float R1 = 10000.0;
byte matriz[NUM_LINHAS][NUM_COLUNAS];
byte matrizSoma[NUM_LINHAS][NUM_COLUNAS];
byte matrizAntiga[NUM_LINHAS][NUM_COLUNAS];
byte matrizCanal[233];
int idxCanal = 0;  // Índice de posição no array
byte matrizTabuleiro[NUM_LINHAS][NUM_COLUNAS];
bool printou = false;

void setup() {
  Serial.begin(9600);
  SerialSoft.begin(115200);
  muxShield.setMode(1, ANALOG_IN);
  muxShield.setMode(2, ANALOG_IN);
  muxShield.setMode(3, ANALOG_IN);
  // SerialSoft.begin(9600);
}

int identificarPeca(float r) {
  if (r < 1100) return 1;
  else if (r < 1300) return 2;
  else if (r < 1550) return 3;
  else if (r < 1900) return 4;
  else if (r < 5100) return 5;
  else if (r < 6850) return 6;
  else if (r < 8900) return 7;
  else if (r < 10100) return 8;
  else if (r < 12500) return 9;
  else if (r < 16000) return 10;
  else return 0;
}

void loop() {
  // 1) Zera soma
  for (int i = 0; i < NUM_LINHAS; i++)
    for (int j = 0; j < NUM_COLUNAS; j++)
      matrizSoma[i][j] = 0;

  // 2) Soma 5 leituras em matrizCanal
  idxCanal = 0;
  for (int p = 0; p < 5; p++) {
    for (int io = 1; io <= 3; io++) {
      for (int i = 0; i < 16; i++) {
        int canal = (io - 1) * 16 + i;
        if (canal == 24) continue;  // pula o lago
        int leitura = muxShield.analogReadMS(io, i);
        float Vout = leitura * (5.0 / 1023.0);
        float R2 = R1 * Vout / (5.0 - Vout);
        if (idxCanal < 233) {
          matrizCanal[idxCanal++] = identificarPeca(R2);
        }
      }
    }
    delay(100);
  }

  // 3) Mapeia matrizCanal para matrizTabuleiro[7][7]
  // (mapeamento conforme seu layout original)
  matrizTabuleiro[0][0] = matrizCanal[0];
  matrizTabuleiro[0][1] = matrizCanal[1];
  matrizTabuleiro[0][2] = matrizCanal[2];
  matrizTabuleiro[0][3] = matrizCanal[3];
  matrizTabuleiro[0][4] = matrizCanal[4];
  matrizTabuleiro[0][5] = matrizCanal[5];
  matrizTabuleiro[0][6] = matrizCanal[6];
  matrizTabuleiro[1][0] = matrizCanal[7];

  matrizTabuleiro[6][6] = matrizCanal[15];
  matrizTabuleiro[6][5] = matrizCanal[14];
  matrizTabuleiro[6][4] = matrizCanal[13];
  matrizTabuleiro[6][3] = matrizCanal[12];
  matrizTabuleiro[6][2] = matrizCanal[11];
  matrizTabuleiro[6][1] = matrizCanal[10];
  matrizTabuleiro[6][0] = matrizCanal[9];
  matrizTabuleiro[5][6] = matrizCanal[8];

  matrizTabuleiro[1][1] = matrizCanal[16];
  matrizTabuleiro[1][2] = matrizCanal[17];
  matrizTabuleiro[1][3] = matrizCanal[18];
  matrizTabuleiro[1][4] = matrizCanal[19];
  matrizTabuleiro[1][5] = matrizCanal[20];
  matrizTabuleiro[1][6] = matrizCanal[21];
  matrizTabuleiro[2][0] = matrizCanal[22];
  matrizTabuleiro[2][1] = matrizCanal[23];

  matrizTabuleiro[5][5] = matrizCanal[31];
  matrizTabuleiro[5][4] = matrizCanal[30];
  matrizTabuleiro[5][3] = matrizCanal[29];
  matrizTabuleiro[5][2] = matrizCanal[28];
  matrizTabuleiro[5][1] = matrizCanal[27];
  matrizTabuleiro[5][0] = matrizCanal[26];
  matrizTabuleiro[4][6] = matrizCanal[25];
  matrizTabuleiro[4][5] = matrizCanal[24];

  matrizTabuleiro[2][2] = matrizCanal[46];  //32
  matrizTabuleiro[2][3] = matrizCanal[45];  //33
  matrizTabuleiro[2][4] = matrizCanal[44];  //34
  matrizTabuleiro[2][5] = matrizCanal[43];  //35
  matrizTabuleiro[2][6] = matrizCanal[42];  //36
  matrizTabuleiro[3][0] = matrizCanal[41];  //37
  matrizTabuleiro[3][1] = matrizCanal[40];  //38
  matrizTabuleiro[3][2] = matrizCanal[39];

  matrizTabuleiro[4][4] = matrizCanal[47];
  matrizTabuleiro[4][3] = matrizCanal[32];  //32
  matrizTabuleiro[4][2] = matrizCanal[33];  //33
  matrizTabuleiro[4][1] = matrizCanal[34];  //34
  matrizTabuleiro[4][0] = matrizCanal[35];  //35
  matrizTabuleiro[3][6] = matrizCanal[36];  //36
  matrizTabuleiro[3][5] = matrizCanal[37];  //37
  matrizTabuleiro[3][4] = matrizCanal[38];  //38
  matrizTabuleiro[3][3] = 11;               // lago

  // 4) Atualiza matriz média final
  for (int i = 0; i < NUM_LINHAS; i++)
    for (int j = 0; j < NUM_COLUNAS; j++)
      matriz[i][j] = matrizTabuleiro[i][j];

  // 5) Detecta mudanças e envia MUD/PUT
  for (int i = 0; i < NUM_LINHAS; i++) {
    for (int j = 0; j < NUM_COLUNAS; j++) {
      byte oldV = matrizAntiga[i][j];
      byte newV = matriz[i][j];
      if (oldV != newV) {
        // converte j->coluna A..G, i->linha 1..7
        char colC = 'A' + j;
        char linC = '1' + i;
        String coord = String(colC) + linC;

        if (oldV != 0 && newV == 0) {
          // peça removida
          Serial.println("MUD " + coord);
          SerialSoft.println("MUD " + coord);
        } else if (oldV == 0 && newV != 0) {
          // peça adicionada
          Serial.println("PUT " + coord);
          SerialSoft.println("PUT " + coord);
        }
        // atualiza a referência
        matrizAntiga[i][j] = newV;
      }
    }
  }
  if (printou == false) {
    String mensagem = "";
    //SerialSoft.print("{{");
    //Serial.print("{{");
    mensagem += "{{";
    for (int i = 0; i < NUM_LINHAS; i++) {
      for (int j = 0; j < NUM_COLUNAS; j++) {
        mensagem += String(matriz[i][j]);
        //SerialSoft.print(matriz[i][j]);
        //Serial.print(matriz[i][j]);
        if (j < NUM_COLUNAS - 1) {
          //SerialSoft.print(",");
         // Serial.print(",");
          mensagem += ",";
        }
      }
      if (i < NUM_LINHAS - 1) {
        //Serial.print("}{");
        mensagem += "}{";
      }
    }
    mensagem += "}}\n";
    SerialSoft.println(mensagem);
    Serial.println(mensagem);
    Serial.println("Matriz enviada por Serial1!");
    printou = true;
  }

  // 6) Aguarda antes da próxima varredura
  delay(3000);
}
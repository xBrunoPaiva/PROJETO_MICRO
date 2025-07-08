if (Serial.available()) {                       // Verifica se há dados disponíveis na Serial
  String texto = Serial.readStringUntil('\n');  // Lê até a próxima quebra de linha
  texto.trim();                                 // Remove espaços em branco extras

  // 1) Comando para limpar os LEDs
  if (texto.equalsIgnoreCase("clear")) {
    limparMatrizDeLEDs();
    Serial.println("Todos os LEDs foram apagados.");
    estado = INI_FLUX;
    return;
  }

  // 2) Se estiver aguardando o destino da peça
  if (estado == EST_AGUARDANDO_DESTINO) {
    char letra = texto.charAt(0);
    colDest = toupper(letra) - 'A';
    linDest = texto.substring(1).toInt() - 1;

    bool ok = false;
    for (int i = 0; i < possCount; i++) {
      if (possMoves[i][0] == linDest && possMoves[i][1] == colDest) {
        ok = true;
        break;
      }
    }

    if (!ok) {
      Serial.println("Destino inválido. Tente novamente.");
      Serial.println("Para onde?");
      return;
    }

    int peca = tabuleiro[LinOri][ColOri];    // Peça a ser movida
    int casa = tabuleiro[linDest][colDest];  // Casa de destino

    if (casa == 0) {  // Movimento normal
      move(LinOri, ColOri, linDest, colDest, peca);
      Serial.println("Peça movida com sucesso!");
    } else {
      int resultado = ataque(peca, casa);                                // Confronto
      if (resultado == 1) move(LinOri, ColOri, linDest, colDest, peca);  // Venceu
      else if (resultado == -1) morte(LinOri, ColOri);                   // Perdeu
      else if (resultado == 2) vence = true;                             // Ganhou o jogo
      else {
        morte(LinOri, ColOri);
        morte(linDest, colDest);
      }  // Empate
    }

    limparMatrizDeLEDs();  // Limpa os LEDs após jogada
    geraMatriz();


    vezP1 = !vezP1;

    if (vezP1) exibeMat2(matrizP1);
    else exibeMat2(matrizP2);

    // Atualiza as matrizes visuais

    imprimeTabuleiro();      // Mostra no Serial
    if (vence) vencedor2();  // Finaliza se alguém ganhou

    // Alterna turno
    estado = INI_FLUX;  // Reinicia estado
    return;
    ////////////////////////////////////////////////////////////////////////////////////////////////
  }

  // 3) Comando para iniciar o jogo
  if (texto.startsWith("iniciar")) {
    inicio = true;
    vence = false;
    tela.fillScreen(TFT_BLACK);
    exibeMat2(matrizP1);
    return;
  }

  // 4) Comandos de controle de LEDs via Serial
  int barra1 = texto.indexOf('/');
  if (barra1 != -1) {
    String acao = texto.substring(0, barra1);
    String coordenadas = texto.substring(barra1 + 1);
    int virgula = coordenadas.indexOf(',');
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
      acender(linha, coluna, 1);
      acender(0, 0, 3);
      acender(1, 0, 3);
      acender(2, 0, 3);
      acender(3, 0, 3);
      acender(4, 0, 3);
      acender(5, 0, 3);
      acender(6, 0, 3);
      acender(7, 0, 3);
    } else if (acao == "apagar") {
      piscarAtivo[idx] = false;
      fita.setPixelColor(idx, 0);
      fita.show();
      Serial.println("LED apagado.");
    } else if (acao == "piscar") {
      piscarAtivo[idx] = true;
      fita.setPixelColor(idx, fita.Color(cor1, cor2, cor3));
      Serial.println("LED piscando.");
    } else {
      Serial.println("Ação inválida. Use: acender, apagar ou piscar");
    }
    return;
  }

  // 5) Comando para mover uma peça
  if (texto.startsWith("mover") && estado == INI_FLUX) {
    limparMatrizDeLEDs();
    char letra = texto.charAt(6);
    int col = toupper(letra) - 'A';
    int lin = texto.substring(7, 8).toInt() - 1;
    String player = texto.substring(9);
    player.trim();

    if (!inicio) {
      Serial.println("Precisa iniciar jogo: iniciar micrombate");
      return;
    }

    int peca = tabuleiro[lin][col];
    bool pecaP1 = (player == "P1" && peca >= 1 && peca <= 5);
    bool pecaP2 = (player == "P2" && peca >= 6 && peca <= 10);
    if ((pecaP1 && !vezP1) || (pecaP2 && vezP1) || !(pecaP2 || pecaP1)) {
      Serial.println("Peca invalida para voce");
      return;
    }

    // calcula os 4 vizinhos
    possCount = 0;
    const int dirs[4][2] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };
    for (int d = 0; d < 4; d++) {
      int nl = lin + dirs[d][0];
      int nc = col + dirs[d][1];
      if (nl < 0 || nl >= NUM_LINHAS || nc < 0 || nc >= NUM_COLUNAS) continue;
      int dest = tabuleiro[nl][nc];
      if (dest == 11) continue;
      if (player == "P1") {
        if (dest >= 1 && dest <= 5) continue;
        possMoves[possCount][0] = nl;
        possMoves[possCount][1] = nc;
        possCount++;

        if (dest == 0) piscaVerde(nl, nc);  // Casa vazia → pisca verde
        else piscaVermelho(nl, nc);         // Inimigo → acende vermelho
      } else if (player == "P2") {
        if (dest >= 6 && dest <= 10) continue;
        possMoves[possCount][0] = nl;
        possMoves[possCount][1] = nc;
        possCount++;

        if (dest == 0) piscaVerde(nl, nc);
        else piscaVermelho(nl, nc);
      }
    }

    if (possCount == 0) {
      Serial.println("Sem movimentos possíveis , escolha uma nova peça para mover.");
      estado = INI_FLUX;
      return;
    }


    LinOri = lin;
    ColOri = col;
    acender(LinOri, ColOri, 3);  // Destaca a peça escolhida com amarelo
    Serial.println("Para onde?");
    estado = EST_AGUARDANDO_DESTINO;
    return;
  }

  // 6) Outros comandos
  if (texto.startsWith("imprime")) {
    geraMatriz();
    exibeMat2(tabuleiro);
  }
  if (texto.startsWith("vencedor")) {
    vencedor2();
  } else if (texto.startsWith("{{")) {
    texto.replace("{", "");
    texto.replace("}", "");
    geraMatriz();
    int linha = 0, coluna = 0, idx = 0;
    while (idx < texto.length() && linha < 7) {
      int espaco = texto.indexOf(',', idx);
      if (espaco == -1) espaco = texto.length();
      String peca = texto.substring(idx, espaco);
      peca.trim();
      int valor = peca.toInt();
      tabuleiro[linha][coluna++] = valor;
      if (coluna == 7) {
        coluna = 0;
        linha++;
        Serial.println("atualizado");
      }
      idx = espaco + 1;
    }
    Serial.println("Matriz recebida!");
    for (int i = 0; i < 7; i++) {
      for (int j = 0; j < 7; j++) {
        Serial.print(tabuleiro[i][j]);
        Serial.print(" ");
      }
      Serial.println();
    }
  }
}

// 7) Lógica de piscar os LEDs
if (millis() - ultimaTroca >= 300) {
  ultimaTroca = millis();
  estadoPisca = !estadoPisca;
  for (int i = 0; i < numLeds; i++) {
    if (piscarAtivo[i]) {
      fita.setPixelColor(i, estadoPisca ? fita.Color(cor1, cor2, cor3) : 0);
    } else if (piscaRed[i]) {
      // alterna vermelho on/off

      fita.setPixelColor(i, estadoPisca ? fita.Color(255, 0, 0) : 0);
    } else if (piscaGreen[i]) {
      // alterna verde on/off

      fita.setPixelColor(i, estadoPisca ? fita.Color(0, 255, 0) : 0);
    }
  }
  fita.show();
}
}

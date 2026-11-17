# Arranger Drum Machine VST3 — compilação automática na nuvem

Este projeto contém o código-fonte de um instrumento VST3 para Windows/Reaper. **Ele ainda não é o plugin compilado.** O workflow incluído permite compilar o VST3 em uma máquina Windows do GitHub Actions e baixar o resultado, sem instalar Visual Studio ou CMake no seu computador.

## Como gerar o VST3 pela internet (GitHub Actions)

1. Crie ou entre em uma conta em [github.com](https://github.com/).
2. Crie um repositório novo, por exemplo `ArrangerDrumMachineVST3`. Para facilitar, deixe-o **Public**; repositórios privados também podem funcionar conforme os limites da conta.
3. Descompacte este ZIP no computador.
4. No repositório do GitHub, use **Add file > Upload files** e envie o conteúdo da pasta `ArrangerDrumMachineVST3` (não envie apenas o ZIP). Inclua também a pasta oculta `.github` e o arquivo `.github/workflows/build-vst3.yml`.
   - Se o navegador não mostrar a pasta `.github`, crie-a no GitHub: **Add file > Create new file**. No campo do nome, digite `.github/workflows/build-vst3.yml` e cole o conteúdo do arquivo `build-vst3.yml` que está neste pacote.
5. Faça **Commit changes** para salvar os arquivos no repositório.
6. Abra a aba **Actions** do repositório. Se aparecer uma solicitação para habilitar workflows, confirme.
7. O workflow também inicia automaticamente após o commit. Para iniciar manualmente, entre em **Actions > Build Arranger Drum Machine VST3 > Run workflow > Run workflow**.
8. Aguarde o processo terminar com um visto verde. Abra a execução concluída e, no final da página, em **Artifacts**, baixe `Arranger-Drum-Machine-VST3-Windows-x64`.
9. Descompacte o arquivo baixado. Dentro estará `Arranger Drum Machine.vst3`.
10. Copie a pasta `Arranger Drum Machine.vst3` para:

   `C:\Program Files\Common Files\VST3\`

11. Abra o Reaper e vá a **Options > Preferences > Plug-ins > VST > Re-scan**. Depois procure por **Arranger Drum Machine** na lista de instrumentos.

## O que a compilação faz

- Usa um runner Windows hospedado pelo GitHub.
- Baixa JUCE 8.0.6 durante a configuração (é necessário que o runner tenha acesso à internet).
- Compila a configuração Release x64 e publica o pacote VST3 como Artifact.
- O Artifact fica disponível por 14 dias; baixe-o para seu computador antes de expirar.

## Conteúdo do instrumento

- 8 estilos: Pop Rock, Disco 80s, Bossa Nova, Hip Hop, Reggae, Ballad, Funk e Samba.
- 4 variações por estilo: Main A, Main B, Fill In 1 e Fill In 2.
- Padrões originais de 16 passos para Kick, Snare, Hi-Hat e Percussion.
- Sons sintetizados de bateria, sem depender de outro instrumento VST.
- BPM 40–240, sincronização com tempo/transporte do host, Run, níveis individuais, Swing e Master.

## Observações importantes

- Esta é uma implementação nova em JUCE, com vozes de bateria sintetizadas. Não é uma cópia bit a bit do processador WASM nem de uma biblioteca de samples original, que não estava incluída no arquivo `.fmx` fornecido.
- A compilação automática não substitui o teste no Reaper. Depois de instalar, confira áudio, sincronismo, estilos e variações. O projeto ainda não foi validado por uma compilação/teste real neste ambiente.

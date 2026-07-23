# Homologação de impressoras

O Santa Agent suporta protocolos padronizados, não uma promessa abstrata de
"todos os modelos". Cada modelo, firmware e configuração deve ser homologado
antes de ser ativado em escala.

## Critérios mínimos por modelo de rede

1. Descoberta por SNMPv3, SNMPv2c autorizado, IPP ou RAW/9100.
2. Inventário: fabricante, modelo, serial e firmware, quando o dispositivo
   os expuser.
3. Estado: pronto, imprimindo, erro e indisponível.
4. Contador total e, se fornecidos pelo fabricante, contador mono/cor.
5. Todos os consumíveis retornados pela Printer-MIB, com níveis coerentes.
6. Regressão após atualização de firmware.

Registre marca, modelo, firmware, credenciais/protocolo permitido e resultado
do teste em um inventário de homologação da Santa Print. HP, Canon, Brother,
Epson, Ricoh, Xerox, Kyocera, Konica Minolta e Lexmark possuem adaptações de
inventário iniciais, mas os OIDs específicos podem variar por família e nunca
devem substituir a validação no equipamento real.

## USB

No Windows, o agente usa o Print Spooler para filas em portas `USB*` e `DOT4*`.
No Linux, usa filas CUPS com URI `usb://` ou `parallel:`. O sistema operacional
normalmente não expõe o contador vitalício ou o nível de toner de uma impressora
USB pura; esses campos devem ficar como desconhecidos em vez de serem estimados.

## Segurança

- Use SNMPv3 e uma conta de leitura dedicada sempre que possível.
- Não use `public` em redes de produção.
- Permita somente a origem do agente no firewall dos dispositivos.
- Verifique cada atualização de firmware antes de liberar para a frota.

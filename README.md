# C++ Agent

Agente proprietário de inventário e monitoramento de impressoras para
equipamentos administrados pelas Empresas de Outsource.

Copyright (c) 2026 Gabriel Barcelos. Consulte [LICENSE](LICENSE).

## Recursos

- Descoberta de impressoras IP por SNMP, IPP/RAW, ARP e mDNS.
- Inventário, status, consumíveis, contadores, rede e exposição de serviços
  inseguros por SNMP/IPP/HTTP.
- Descoberta inicial de impressoras USB locais no Windows via Print Spooler.
- API REST autenticada, histórico em SQLite e alertas por webhook e SMTP.
- Adaptadores genéricos baseados em padrões e extensões por fabricante.

Não declare compatibilidade com todos os modelos: modelos e versões de
firmware devem ser homologados antes da implantação. O agente usa SNMP/IPP
padronizados quando disponíveis e aplica extensões específicas para alguns
fabricantes.

## Instalação

1. Instale CMake 3.20+, um compilador C++17, vcpkg e Net-SNMP.
2. Instale as dependências declaradas em `vcpkg.json` e disponibilize
   Net-SNMP para o CMake. No Linux, instale também `pkg-config` e os headers
   de desenvolvimento do Net-SNMP.
3. Copie `config.example.toml` para `config.toml`, defina credenciais e gere
   um token forte para a API.
4. Restrinja as ACLs do arquivo de configuração e do diretório de dados ao
   usuário de serviço do agente.
5. Compile e execute:

   ```powershell
   cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=<vcpkg>/scripts/buildsystems/vcpkg.cmake
   cmake --build build --config Release
   .\build\Release\santa_agent.exe --config .\config.toml
   ```
- Prefira SNMPv3; segredos nunca devem ser enviados para a API.
- Mantenha a API em `127.0.0.1` ou atrás de VPN/reverse proxy autenticado.
- Autorize somente as faixas de rede dos clientes e use firewall local.
- Teste cada modelo, firmware e consumível antes de considerá-lo homologado.
- 

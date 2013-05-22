IMPLEMENTACE ZŘETĚZENÉHO RDT
Projekt do předmetu IPK/2011
autor: Jan Dušek <xdusek17@stud.fit.vutbr.cz>

Vytvořeny byly dva programy client a server.

Klient:
Použití: rdtclient -s SPORT -d DPORT
Vytvoří spojení na vzdálený port DPORT. Naslouchá na lokalním porty SPORT
komunikaci od serveru. Čte řádky o maximalní délce 80 znaků ze standardního
vstupu a posílá je serveru. Po odeslání všech dat ze vstupu se ukončí.

Server:
Použití: rdtserver -S SPORT -d DPORT
Naslouchá na portu SPORT a na DPORT zasílá odpovědi. Klient zasílá řádky
a server je vypisuje na standardní výstup. Po příjmutí všech dat se ukončí.

Protokol RDT:
Používá se Go-Back-N s velikostí okna 10 na strane serveru i klienta.
ACK potvrzuje vsechny pakety mensí jak jeho číslo. Server posílá vždy
ACK paketu, který chce příjmout tedy pokud přijde paket mimo okno pošle ACK
stejné jako minule, pokud přijde paket do okna uloží si ho a pošle ACK číslo
nejmenšího paketu, který ještě nedorazil, v okně, potvrzene pakety zpracuje a
posune okno. Tím, že se i na straně serveru používá okno 10, zrychluje se
přenos pokud pakety dochází mimo pořadí.

Čísla paketů jsou z intervalu 0..2*WINDOW_SIZE aby nedocházelo k disinterpretaci
paketu serverem.

Dle zadání je maximální spoždění na lince 500ms proto klient pokud neobdrží
během 500ms žádnou odpověd od serveru zasílá všechny již zaslané a nepotvrzené
pakety znovu.

Hlavička RDT paketu má 32bitů.
[---checksum---|--pack-num--|type]
[     16       |     13     |  3 ]

Pro výpočet kontrolního součtu se používá internet checksum z RFC 1071. Součet
se vypočítává z druhých 16bitu hlavičky a dat paketu.
Protokol má několik typů paketů:
    DAT : datový paket je jediný který obsahuje po hlavičce data.
    EST : tento paket zasílá klient pro navázání spojení
    CLO : tento paket uzavírá spojení
    DACK: potvrzení příjmu datového paketu. V současné implementaci potvrzuje 
          pakety mensi jak pack-num
    EACK: potvrzení navázání spojení zasílá server pokud akceptuje žádost o
        : vytvoření spojení od klienta


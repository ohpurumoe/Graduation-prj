#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

/*
nvme 기본 테스트
파일 시스템 올린 상태로 read, write 진행
*/


char filename[8][8]={"README","README2","README3","README4","README5"};

char *p[8];
int v[8];

int random[700]=
{937432429 ,529339248 ,965699715 ,1781711590 ,1715076789 ,1738179532 ,736396899 ,732051052 ,1038478340 ,2014919264 ,981533440 ,108165064 ,283288278 ,1285977486 ,298445791 ,829170461 ,1131232316 ,1853763584 ,735301336 ,1993833673 ,38621122 ,1947597722 ,70601867 ,1834848294 ,1367119786 ,1522023018 ,831770209 ,2018399691 ,1308300331 ,915685210 ,541577638 ,98249112 ,1445024459 ,1507277354 ,1879960702 ,1012617600 ,1097973238 ,468873954 ,1744668652 ,2136451578 ,336309570 ,578718444 ,97132994 ,619597848 ,1864695931 ,395578785 ,1448768309 ,848444599 ,101858722 ,36585998 ,694794624 ,140479844 ,1984183720 ,765396491 ,1975328138 ,1203819858 ,139935861 ,659614699 ,1074735901 ,1448236192 ,1575299910 ,1616313540 ,1546485304 ,872840721 ,976107246 ,1278962358 ,1885458321 ,2074080484 ,1747836312 ,1482643325 ,2063048414 ,2084145882 ,2061361770 ,12697760 ,556260083 ,1778574053 ,408276546 ,2005028392 ,479535004 ,510135268 ,2041614390 ,1174329628 ,650615112 ,1878314462 ,1939726119 ,478459602 ,934650672 ,2079661980 ,1138074302 ,2009386574 ,1380414524 ,565890564 ,1478216466 ,779416180 ,1438731285 ,306840064 ,2058378538 ,1176705958 ,233436900 ,1658731203 ,511865635 ,149001666 ,1595393437 ,425743757 ,161699426 ,4169872 ,56834162 ,569975972 ,2009198265 ,536369166 ,1080111240 ,1903329007 ,1710698794 ,1730726352 ,1634159822 ,1502941265 ,61702307 ,421326846 ,1435119597 ,1199776609 ,283229772 ,668050473 ,1765667173 ,1761446238 ,1447466653 ,1056914810 ,2068286302 ,1358361544 ,86137120 ,154239554 ,869609099 ,598002755 ,303241220 ,317518888 ,1023746513 ,464940647 ,321688761 ,1080580675 ,1034916619 ,183403378 ,1616949842 ,2115027860 ,2086732385 ,1180164988 ,1698270564 ,1573408559 ,535622606 ,1759972871 ,1994735406 ,1970742203 ,812265832 ,130481530 ,491309029 ,430449357 ,1891927769 ,1938775682 ,1487364167 ,1812730423 ,1149653578 ,1573501287 ,1966969978 ,2019262677 ,24020395 ,122727550 ,189297918 ,1047766908 ,587668197 ,510986679 ,2128347583 ,1622584817 ,694390057 ,1597813777 ,1590129029 ,633638794 ,630495118 ,1140915945 ,59563706 ,1166117724 ,753405169 ,2054299112 ,989376279 ,1565671001 ,37296994 ,1480685308 ,1996120359 ,1929224763 ,1271977343 ,1336000878 ,1594471539 ,274147273 ,762018518 ,1413957869 ,145926303 ,786038913 ,1536685419 ,335224221 ,1833805821 ,2124353617 ,846210900 ,1814669756 ,1599454786 ,1540600957 ,1264999886 ,1042100167 ,26756103 ,1895495004 ,35532464 ,86319809 ,914129080 ,788937633 ,2140618921 ,1903505359 ,207124987 ,30432268 ,1236707020 ,55761698 ,1959657031 ,361200715 ,1391762576 ,1406644922 ,635347988 ,6297446 ,673119143 ,781274291 ,792336359 ,62320915 ,1116498512 ,478658532 ,39190884 ,1962709412 ,145844641 ,1638645670 ,1355826721 ,1410844527 ,533262189 ,1382582825 ,1158855883 ,568794653 ,1468902634 ,2072984963 ,1357732287 ,1462037908 ,1829006674 ,1564857274 ,1492470176 ,918230046 ,1620618972 ,1304643559 ,1279430761 ,864897900 ,563804834 ,1914778750 ,871195347 ,1236923977 ,548569393 ,1663531706 ,1299244892 ,1665067906 ,2142190239 ,1338435776 ,1480293670 ,140551232 ,829597798 ,688636744 ,1551395759 ,1362859987 ,2071219569 ,562767994 ,1931654641 ,1392638555 ,488269309 ,1141903280 ,707192815 ,169792335 ,559276906 ,52179343 ,1088022382 ,32412230 ,1356822903 ,219969495 ,897310130 ,1920627737 ,2134748245 ,1768505477 ,1010068066 ,535833991 ,1284553536 ,161829311 ,53418249 ,1279260127 ,1500265087 ,1533711919 ,1419811359 ,182379238 ,74865015 ,823723470 ,1545239225 ,2146084584 ,1386491464 ,1329410218 ,1391239492 ,1874760773 ,323829850 ,2098432307 ,2044553108 ,883106756 ,3128003 ,985091842 ,915518986 ,1359950906 ,1205061338 ,1812829117 ,1133094995 ,1192325935 ,1433850946 ,2143163061 ,1728159926 ,570920834 ,157508724 ,1781578175 ,1850180961 ,1657773812 ,1167806447 ,1122508672 ,1840153050 ,1242671462 ,1946232142 ,1237908627 ,1241272399 ,1185239958 ,419835198 ,485028243 ,912517083 ,743665048 ,435976902 ,809586544 ,1626771805 ,439104905 ,1794678386 ,394807143 ,1799055811 ,852256076 ,60152612 ,784667158 ,2044582012 ,1494003559 ,780346572 ,1625258290 ,2064924393 ,937855296 ,1259352818 ,1767621707 ,448145460 ,279675617 ,742646731 ,140814862 ,1522347079 ,541395226 ,1378723490 ,616135830 ,1726635184 ,1798558688 ,1101164073 ,491668620 ,394740088 ,1537140976 ,1301255164 ,2021511893 ,1976245881 ,948449902 ,268835389 ,1627818045 ,1800705979 ,328988001 ,265001555 ,1697804343 ,1822991560 ,1045348127 ,1175578985 ,1740432306 ,1983203424 ,287448155 ,1360570365 ,283865236 ,567123772 ,2103217096 ,424680099 ,2089470852 ,497128674 ,1803403589 ,558123034 ,76280211 ,1454478629 ,1659287108 ,567948831 ,1849218717 ,1048944436 ,1869203995 ,1723246963 ,877706669 ,670170249 ,1992082352 ,358041066 ,323392580 ,173586705 ,623042622 ,2021196923 ,1996578266 ,1668390749 ,1049292261 ,1589526924 ,1504110525 ,1336740416 ,802613641 ,1787975762 ,1903864189 ,758347089 ,65172213 ,1845851393 ,1255475764 ,1868575802 ,256490779 ,1331755975 ,1175570783 ,1915777887 ,1899704806 ,877305852 ,817238675 ,1621425153 ,453069167 ,1694945345 ,144111754 ,297667871 ,2052986411 ,467504335 ,471254577 ,528545385 ,341217610 ,320349195 ,49452487 ,1390509871 ,1909876119 ,1553563012 ,579766640 ,565006112 ,1194055126 ,336147181 ,1323353201 ,1259227339 ,34514926 ,431345317 ,980319493 ,291005705 ,1763101292 ,8406628 ,59299945 ,1515322450 ,885712481 ,876538620 ,989263955 ,1338781648 ,424000317 ,1133375710 ,1636449520 ,329503081 ,1600880045 ,2107704097 ,858048466 ,1942097655 ,280569644 ,907500953 ,1185123879 ,42962115 ,313580318 ,1764890519 ,607968227 ,1507635444 ,2101037700 ,1931321428 ,619379136 ,2135552626 ,215183098 ,1599698629 ,279074683 ,1978284390 ,1608105258 ,338374628 ,1346123193 ,346334091 ,1214913249 ,187903500 ,1685115739 ,1638913566 ,1321279210 ,1174081611 ,1968416647 ,774675607 ,1134302060 ,678981466 ,569289615 ,1414871704 ,1586482419 ,1754413494 ,1457833819 ,1900062737 ,1371820365 ,2065802046 ,1260214534 ,1325374417 ,1849639827 ,1879593670 ,1313443395 ,2064822925 ,1331808651 ,1592518078 ,1895623667 ,792430261 ,1930892707 ,1094263212 ,1138764352 ,998322308 ,1282166713 ,676396444 ,489752226 ,455962275 ,1850478055 ,310685226 ,1230637883 ,837296468 ,989666692 ,1799927498 ,104684524 ,428665463 ,1406857344 ,1562518344 ,181244553 ,631194061 ,1480836742 ,1441459087 ,1956568478 ,1182992921 ,1173569109 ,1122528225 ,1100332198 ,357894112 ,567562655 ,848472218 ,1150324374 ,350971714 ,1942735430 ,141605078 ,1349294022 ,1077418495 ,818001522 ,1839046249 ,1533380771 ,520995930 ,2247827 ,616535006 ,1358292398 ,991914519 ,268978856 ,1462976922 ,1420579982 ,1675836200 ,878011618 ,1601824535 ,159546613 ,211364713 ,895799974 ,2116115091 ,1394357634 ,2069369083 ,1091159668 ,347206185 ,279779548 ,1658722323 ,1195678403 ,1430103922 ,2009694038 ,990930185 ,1571709000 ,1211504412 ,2068348681 ,242226875 ,903067013 ,1454245804 ,763222805 ,905314840 ,2070780810 ,2121515203 ,1897229359 ,192276018 ,1437008477 ,1170325694 ,1868112218 ,167536448 ,624666581 ,2027658831 ,378901161 ,1520466556 ,1996290274 ,1773258795 ,1442351991 ,939966294 ,2120464980 ,1722131539 ,451204969 ,1168659735 ,1004751813 ,313415359 ,12106273 ,428977166 ,1524919772 ,2080454954 ,671204041 ,280503137 ,1387217110 ,1434426846 ,1185817978 ,1310514272 ,1408458401 ,935563689 ,1502790290 ,697983230 ,2105889383 ,1223418860 ,865519678 ,583072317 ,1103594043 ,1244420839 ,2103538873 ,952400669 ,870195987 ,1398407216 ,1892366963 ,843177319 ,973055108 ,196088284 ,2011837055 ,1977806921 ,509503644 ,2023943328 ,259300439 ,2034423416 ,1956914634 ,930504480 ,167442905 ,1196648096 ,217447678 ,1353260883 ,359678720 ,1625906079 ,141340925 ,1862469010 ,176405662 ,99746660 ,938404222 ,1041925340 ,682818977 ,2041998265 ,138862532 ,638874202 ,846915286 ,1009058519 ,2037281419 ,591798601 ,1852235838 ,862852879 ,787886885 ,1716589245 ,693176152 ,1297390529 ,1593048925 ,952476592 ,1184330297 ,1402479911 ,1882981072 ,1351773203 ,451644359 ,2100428751 ,557550438 ,811323079 ,1578851182 ,698891363 ,526308441 ,1755256844 ,798638024 ,1464712663 ,649698537 ,1481457001 ,1359227280 ,788561069 ,2120331204 ,58658918 ,1797619588 ,2010128975 ,650457519 ,1502371778 ,725498206};


int randpower[700];
int randcube[700];
int randcnt = 0;
/*
iter는 input 실행 횟수
mode는 캐시 사용여부 (0 사용안함, 1 사용)
*/

int input_sr(int iter, int mode)
{
    int tmp1 = get_ticks();
    for (int i = 0; i < 5; i++){
        if (mode == 0) v[i] = open(filename[i],O_RDWR);
        else v[i] = caching_open(filename[i],O_RDWR);
    }  
    for (int i = 0; i < iter; i++){
        for (int j = 0; j < 5; j++){
            if(mode == 0) {
                fileoffset(v[j%5],0);
                read(v[j%5],p[j%5],4096*3);
            }
            else caching_read(v[j%5],p[j%5],4096*3,0);
        }
    }
    int tmp2 = get_ticks();
    for (int i = 0; i < 5; i++){
        if(mode == 0) close(v[i]);
        else caching_close(v[i]);
    }
    return tmp2- tmp1;
}


int input_sw(int iter, int mode)
{
    int tmp1 = get_ticks();
    for (int i = 0; i < 5; i++){
        if (mode == 0) v[i] = open(filename[i],O_RDWR);
        else v[i] = caching_open(filename[i],O_RDWR);
    }  
    for (int i = 0; i < iter; i++){
        for (int j = 0; j < 5; j++){
            if(mode == 0) {
                fileoffset(v[j%5],0);
                write(v[j%5],p[j%5],4096*3);
            }
            else caching_write(v[j%5],p[j%5],4096*3,0);
        }
    }
    int tmp2 = get_ticks();
    for (int i = 0; i < 5; i++){
        if(mode == 0) close(v[i]);
        else caching_close(v[i]);
    }
    return tmp2- tmp1;
}

int abs(int a){
    if (a < 0) return -a;
    return a;
}

int input_rr(int rand, int iter, int mode)
{
    int tmp1 = get_ticks();
    for (int i = 0; i < 5; i++){
        if (mode == 0) v[i] = open(filename[i],O_RDWR);
        else v[i] = caching_open(filename[i],O_RDWR);
    }  
    for (int i = 0; i < iter; i++){
        for (int j = 0; j < 5; j++){
            if(rand <= abs(random[randcnt]%10)){
                if(mode == 0) {
                    fileoffset(v[j%5],0);
                    read(v[j%5],p[j%5],4096*3);
                }
                else caching_read(v[j%5],p[j%5],4096*3,0);            
            }
            else {
                if(mode == 0) {
                    fileoffset(v[random[randcnt]%5],randpower[randcnt] % (3*4096));
                    read(v[random[randcnt]%5],p[random[randcnt]%5],randcube[randcnt] % (3* 4096));
                }
                else {
                    //printf(1, "%i is %d j is %d randcnt %d cube %d power %d\n",i,j,randcnt ,randcube[randcnt]%(3*4096), randpower[randcnt]%(3*4096));
                    caching_read(v[random[randcnt]%5],p[random[randcnt]%5],randcube[randcnt] % (3* 4096),randpower[randcnt] % (3*4096));
                }
            }
            randcnt++;
            randcnt %= 700;
        }
    }
    int tmp2 = get_ticks();
    for (int i = 0; i < 5; i++){
        if(mode == 0) close(v[i]);
        else caching_close(v[i]);
    }    
    return tmp2- tmp1;
}

int input_rw(int rand , int iter, int mode)
{
    int tmp1 = get_ticks();
    for (int i = 0; i < 5; i++){
        if (mode == 0) v[i] = open(filename[i],O_RDWR);
        else v[i] = caching_open(filename[i],O_RDWR);
    }  
    for (int i = 0; i < iter; i++){
        for (int j = 0; j < 5; j++){
            if(rand <= abs(random[randcnt]%10)){
                if(mode == 0) {
                    fileoffset(v[j%5],0);
                    write(v[j%5],p[j%5],4096*3);
                }
                else caching_write(v[j%5],p[j%5],4096*3,0);            
            }
            else{
                if(mode == 0) {
                    fileoffset(v[random[randcnt]%5],randpower[randcnt] % (3*4096));
                    write(v[random[randcnt]%5],p[random[randcnt]%5],randcube[randcnt] % (3* 4096));
                }
                else caching_write(v[random[randcnt]%5],p[random[randcnt]%5],randcube[randcnt] % (3* 4096),randpower[randcnt] % (3*4096));
            }
            randcnt++;
            randcnt %= 700;
        }
    }
    int tmp2 = get_ticks();
    for (int i = 0; i < 5; i++){
        if(mode == 0) close(v[i]);
        else caching_close(v[i]);
    }    
    return tmp2- tmp1;
}

int
main(int argc, char* argv[])
{   

    for (int i = 0; i < 700; i++){
        randpower[i] = (random[i]*random[i])%(3*4096);
        randcube[i] = (randpower[i]*random[i])%(3*4096);
    }
    for (int i = 0; i < 700; i++){
        random[i] %= 7;
    }

    for (int i = 0; i < 700; i++){
        if(randpower[i] < 0) randpower[i] *= -1;
        if(randcube[i] < 0) randcube[i] *= -1;        
        if (random[i] < 0) random[i] *= -1;
    }
    for (int i = 0; i < 700; i++){
        if(randcube[i]  + randpower[i]  > 3*4096){
            randcube[i] = (3*4096) - randpower[i] %(3*4096);
        }
    }
    for (int i = 0; i < 7; i++){
        p[i] = malloc(4096*3);
        for (int j = 0; j < 3*4096; j++){
            p[i][j] = 'a'+i;
        }
    }    
   // printf(1,"Cache Test\n");
   // printf(1,"sequential read without pagecache --- ticks : %d\n", input_sr(100,0) );
   // printf(1,"sequential read with pagecache --- ticks : %d\n", input_sr(100,1) );
    //nvme_setting();
    //printf(1,"sequential write without pagecache --- ticks : %d\n", input_sw(100,0) );
    //printf(1,"sequential write with pagecache --- ticks : %d\n", input_sw(100,1) );
    //nvme_setting();    
////////////////////////////////////////////////////////////////////////////////////////
    //printf(1,"random read without pagecache --- ticks : %d\n", input_rr(100,0) );
    //printf(1,"random read with pagecache --- ticks : %d\n", input_rr(100,1) );
    //nvme_setting();
    //printf(1,"random write without pagecache --- ticks : %d\n", input_rw(8,100,0) );
    printf(1,"random write with pagecache --- ticks : %d\n", input_rw(8,100,1) );
    nvme_setting(); 
    
    exit();
}

#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <math.h>
#include <chrono>
#include <random>


using namespace std;


class Klient {
public:
    int numer, x, y, zapotrzebowanie, od_kiedy, do_kiedy, czas_obslugi;

    Klient(int NO, int XCOORD, int YCOORD, int DEMAND, int READY_TIME, int DUE_TIME, int SERVICE_TIME) {
        numer = NO;
        x = XCOORD;
        y = YCOORD;
        zapotrzebowanie = DEMAND;
        od_kiedy = READY_TIME;
        do_kiedy = DUE_TIME;
        czas_obslugi = SERVICE_TIME;
    }

    int daj_numer() {
        return numer;
    }
    int daj_x() {
        return x;
    }
    int daj_y() {
        return y;
    }
    int daj_zapotrzebowanie() {
        return zapotrzebowanie;
    }
    int od_kiedy_otwarty() {
        return od_kiedy;
    }
    int do_kiedy_otwarty() {
        return do_kiedy;
    }
    int daj_czas_obslugi() {
        return czas_obslugi;
    }

    // liczy odległość między 2 klientami(i/lub magazynem)
    double licz_odleglosc(Klient do_kogo) {
        int dx = do_kogo.daj_x() - this->x;
        int dy = do_kogo.daj_y() - this->y;
        return sqrt(pow(dx, 2) + pow(dy, 2));
    }
private:
};



vector<Klient> klienci; // globalny wektor do którego będą zapisani magazyn i wszyscy klienci(magazyn może być zapisany jako klient 0)
vector<Klient> klienci_kopia; //kopia wektora klientów używana przez algorytm tabu
int ladunek;//globalna zmienna zawierająca pojemność ciężarówek


// wczytywanie danych z pliku wejściowego
int wczytanie_pliku(string nazwa, int* tab, int ile_linii) {
    ifstream plik_wejsciowy;
    string linia;
    int k, x, y, zap, otw, zam, ser;
    plik_wejsciowy.open(nazwa);
    if (!plik_wejsciowy.good()) {
        return -1;
    }

    for (int i = 0; i < 4; i++) {
        getline(plik_wejsciowy, linia); // 5 linia to interesujące nas dane, poprzednie pomijamy
        if (linia.size() < 4) i--;
    }
    stringstream bufor1(linia);
    bufor1 >> tab[0] >> tab[1];

    for (int i = 0; i < 2; i++) {
        getline(plik_wejsciowy, linia); // pomijamy 4 zbędne linie
        if (linia.size() < 4) i--;
    }

    // gdy nie czytamy wszystkich, dodaje jedną do odczytania magazynu("ile_linii" klientów + 1 magazyn)
    if (ile_linii != -1) ile_linii++;

    while (getline(plik_wejsciowy, linia) && ile_linii != 0) {
        stringstream bufor2(linia);
        if (bufor2.str().size() < 10) continue;
        bufor2 >> k >> x >> y >> zap >> otw >> zam >> ser;
        klienci.push_back(Klient(k, x, y, zap, otw, zam, ser));
        ile_linii--;
    }
    plik_wejsciowy.close();
    return 0;
}


// sprawdza czy da się obsłużyć konkretnego klienta, pod względem towaru w ciężarówce, czasu dojazdu do klienta i powrotu do magazynu
bool da_sie_obsluzyc(Klient obecny, Klient cel, double ile_minelo, int ile_zostalo) {
    double czas = ile_minelo + obecny.licz_odleglosc(cel);
    
    bool czy_wystarczy = (ile_zostalo >= cel.daj_zapotrzebowanie());    // sprawdza, czy towaru w ciężarówce starczy na konkretnego klienta
    bool czy_dojedzie = (cel.do_kiedy_otwarty() >= czas);   // sprawdza czy zdąży dojechać do klienta(lub magazynu) przed jego zamknięciem
    
    if (czas < cel.od_kiedy_otwarty()) {    // gdy dojedzie przed otwarciem klienta "czeka" do momentu otwarcia
        czas = cel.od_kiedy_otwarty();
    }
    czas = czas + cel.daj_czas_obslugi();
    bool zdazy_wrocic = (klienci[0].do_kiedy_otwarty() >= czas + cel.licz_odleglosc(klienci[0]));   // sprawdza czy zdąży wrócić do magazynu po obsłużeniu
    
    return (czy_wystarczy&&czy_dojedzie&&zdazy_wrocic);
}


double najlepszy_lokalnie_klient(Klient obecny, Klient cel) {
    return obecny.licz_odleglosc(cel);
}


bool czy_dopuszczalne() { // sprawdza, czy da się obsłużyć każdego klienta niezależnie
    for (Klient x : klienci) {
        if (x.daj_numer() == 0) continue; // klienci[0] to magazyn - pomijamy

        if (x.licz_odleglosc(klienci[0]) >= x.od_kiedy_otwarty()) {
            // gdy ciężarówka dojedzie po otwarciu, sprawdza czy zdążyła przed zamknięciem i czy zdąży wrócić do magazynu
            if ((x.licz_odleglosc(klienci[0]) > x.do_kiedy_otwarty()) || (x.licz_odleglosc(klienci[0]) * 2 + x.daj_czas_obslugi() > klienci[0].do_kiedy_otwarty())) {
                return false;
            }
        }
        else {
            // gdy dojedzie przed czasem, sprawdza czy zdąży wrócić do magazynu po odsłużeniu klienta
            if (x.licz_odleglosc(klienci[0]) + x.daj_czas_obslugi() + x.od_kiedy_otwarty() > klienci[0].do_kiedy_otwarty()) {
                return false;
            }
        }
    }
    return true;
}


struct wynik { // struktura używana do zapisania różnych rozwiązań
    vector<vector<int>> wszystkie_trasy;
    int ilosc_tras;
    double czas;
    
};


wynik zachlanny(int cap) {
    if (!czy_dopuszczalne()) {
        return (wynik){vector<vector<int>> (), -1, -1};
    }

    int przejechane_trasy = 0;
    double sumaryczny_czas = 0.0;
    double czas = 0.0;
    double ladownosc = cap;
    Klient obecny_klient = klienci[0];
    vector<vector<int>> trasy;
    vector<int> trasa;
    trasy.push_back(trasa);


    while (true) {
        if (klienci.size() == 1) {  //kiedy w tablicy zostanie sam magazyn (klienci[0])
            while(trasy.back().empty())
                trasy.pop_back();

            wynik x;
            x.wszystkie_trasy = trasy;
            x.czas = sumaryczny_czas;
            x.ilosc_tras = trasy.size();
            return x;
        }

        int nastepny_klient = 1; // pomijamy magazyn (klienci[0] = magazyn), przypisujemy pierwszego klienta z listy
        for (int i = 2; i < (int)klienci.size(); i++) {  // szukanie najlepszego lokalnego następnego klienta
            if (da_sie_obsluzyc(obecny_klient, klienci[i], czas, ladownosc) &&
                najlepszy_lokalnie_klient(obecny_klient, klienci[i]) 
                < najlepszy_lokalnie_klient(obecny_klient, klienci[nastepny_klient])) {
                nastepny_klient = i;
            }
        }

        // gdy nie znajdzie następnego pasującego klienta i piewszy z listy(ustawiany domyślnie) nie może być obsłużony,
        // przypisuje wartość -1, która reprezentuje brak następnego klienta
        if (nastepny_klient == 1 && !da_sie_obsluzyc(obecny_klient, klienci[1], czas, ladownosc)) {
            nastepny_klient = -1;
        }

        if (nastepny_klient != -1) {    // jeśli znalazło następnego klienta...
            trasy[przejechane_trasy].push_back(klienci[nastepny_klient].daj_numer()); // dodajemy jego id do obecnej trasy

            sumaryczny_czas = sumaryczny_czas + obecny_klient.licz_odleglosc(klienci[nastepny_klient]);
            czas = czas + obecny_klient.licz_odleglosc(klienci[nastepny_klient]);

            if (klienci[nastepny_klient].od_kiedy_otwarty() > czas) {   // gdy dojedzie przed czasem otwarcia klienta, "czeka" i dodaje czas oczekiwania
                sumaryczny_czas = sumaryczny_czas + (klienci[nastepny_klient].od_kiedy_otwarty() - czas);
                czas = klienci[nastepny_klient].od_kiedy_otwarty(); 
            }

            ladownosc = ladownosc - klienci[nastepny_klient].daj_zapotrzebowanie();

            sumaryczny_czas = sumaryczny_czas + klienci[nastepny_klient].daj_czas_obslugi(); 
            czas = czas + klienci[nastepny_klient].daj_czas_obslugi();

            obecny_klient = klienci[nastepny_klient];   // następny klient staje sie obecnym klientem
            klienci.erase(klienci.begin() + nastepny_klient);   // usunięcie obsłużonego (aktualnie obecnego) klienta
        }

        if (ladownosc == 0 || nastepny_klient == -1) { // jeśli nie ma następnego klienta lub skończył się ładunek następuje podsumowanie trasy 
            przejechane_trasy++;    
            sumaryczny_czas = sumaryczny_czas + obecny_klient.licz_odleglosc(klienci[0]);
            czas = 0.0; // resetujemy czas trasy
            ladownosc = cap; // reset ładowności
            obecny_klient = klienci[0]; // ciężarówka zaczyna z magazynu
            vector<int> trasa;
            trasy.push_back(trasa);
        }

        if (klienci.size() == 1) {  // gdy skończyli się wszyscy klienci(finalizujemy ostatnią trasę)
            przejechane_trasy++;   
            sumaryczny_czas += obecny_klient.licz_odleglosc(klienci[0]);
        }
    }
}

// sprawdzenie dopuszczalności tras
// sprawdza czasy dojazdów i obsługi klientów, oraz czy wróci do magazynu
bool droga_mozliwa(vector<int> trasa) {
    int calkowity_ladunek = 0;
    double czas = 0;
    Klient obecny_klient = klienci_kopia[0]; 

    for(int nr: trasa) {
        Klient nastepny_klient = klienci_kopia[nr];
        
        czas = czas + obecny_klient.licz_odleglosc(nastepny_klient);
        calkowity_ladunek = calkowity_ladunek + obecny_klient.daj_zapotrzebowanie(); 
    
        if(nastepny_klient.od_kiedy_otwarty() > czas) {
            czas = nastepny_klient.od_kiedy_otwarty();
        }
        else if (nastepny_klient.do_kiedy_otwarty() < czas) { // jeśli nie da się dojechać do klienta od razu kończymy sprawdzanie
            return false;
        }

        czas = czas + nastepny_klient.daj_czas_obslugi();
        obecny_klient = nastepny_klient;
    }

    calkowity_ladunek = calkowity_ladunek + obecny_klient.daj_zapotrzebowanie();
    
    if (calkowity_ladunek <= ladunek) { // jeśli ładunek i czas się zgadza zwracamy true
        if (czas + obecny_klient.licz_odleglosc(klienci_kopia[0]) < klienci_kopia[0].do_kiedy_otwarty())
            return true;
    }
    return false;

}

// mieszanie trasy wg reguły two-opt
// "wycinamy" kawałek ze środka trasy i podłączamy go odwrotnie 
vector<int> two_opt_swap(int cut1, int cut2, vector<int> trasa) { 
    vector<int> test_trasa;
    
    for (int i = 0; i < cut1; i++) {
        test_trasa.push_back(trasa[i]);
    }

    for (int i = cut2 - 1; i > cut1 + 1; i--) {
        test_trasa.push_back(trasa[i]);
    }

    for (int i = cut2; i < (int)trasa.size(); i++) {
        test_trasa.push_back(trasa[i]);
    }

    if (droga_mozliwa(test_trasa)) { // sprawdzamy czy nowa trasa jest możliwa
        return test_trasa;
    }
    return trasa;
}

// zliczamy czas/koszt przejechania pojedyńczej trasy
double koszt_trasy(vector<int> trasa) {
    double czas = 0;
    Klient obecny_klient = klienci_kopia[0];
    
    for (int nr: trasa) {
        Klient nastepny_klient = klienci_kopia[nr];

        czas = czas + obecny_klient.licz_odleglosc(nastepny_klient); // czas dojazdu do punktu
        if (nastepny_klient.od_kiedy_otwarty() > czas ) {
            czas = nastepny_klient.od_kiedy_otwarty();
        }
        czas = czas + nastepny_klient.daj_czas_obslugi(); // obsługa punktu
        obecny_klient = nastepny_klient;
    }
    return czas + obecny_klient.licz_odleglosc(klienci_kopia[0]); // dodajemy czas powrotu do magazynu
}

// algorytm two-opt -> szukanie najlepszej modyfikacji aktualnej trasy
// w podwójnej pętli for sprawdzamy każdą opcje zamiany trasy, 
// jeśli uzyskana trasa jest lepsza od obecnie najlepszej nadpisuje ona obecnie najlepszą.
// powtarzamy proces w pętli while na każdej nowej trasie, aż do momentu gdy nie znajdzie sie lepsza modyfikacja
void two_opt(vector<int> trasa) {
    vector<int> nowa_trasa;
    nowa_trasa = trasa; 
    while(1) {
        
        double koszt_max = koszt_trasy(trasa);
        bool czy_lepsza = false;
        for(int i = 0; i < (int)trasa.size() - 1; i++) {
            for(int j = i + 1; j <= (int)trasa.size() - 1; j++) {
                vector<int> nowa_trasa = two_opt_swap(i, j, trasa);
                if(double nowy_koszt = koszt_trasy(nowa_trasa) < koszt_max) {
                    trasa = nowa_trasa;
                    koszt_max = nowy_koszt;
                    czy_lepsza = true;
                }
            }
        }
        if(!czy_lepsza) {// gdy trasu nie da się już lepiej ułożyć, kończymy
            break;
        }
    }
}


// wymienia klientów między dwoma ustalonymi trasami.
// sprawdza każdą możliwą pare i gdy znajdzie potencjalnie lepszą
// losuje z ustalonym prawdopodobieństwem czy zostawia taką zmiane, czy szuka następnych
int wymien_klientow(vector<int>& trasa1, vector<int>& trasa2) {

    random_device rd;    //Will be used to obtain a seed for the random number engine
    mt19937 gen(rd());   //Standard mersenne_twister_engine seeded with rd()
    uniform_real_distribution<> dis(0.0, 1.0);
    // ^^ generator liczb losowych znaleziony w internecie // lepszy niż zwykły rand()
    int i_last = -1, j_last = -1;

    for (int  i = 0; i < (int)trasa2.size(); i++) {
        for (int j = 0; j < (int)trasa1.size(); j++) {
            swap(trasa2[i],trasa1[j]);
            if (droga_mozliwa(trasa2) && droga_mozliwa(trasa1)) {
                if(0.15 >= dis(gen)) {  // do regulowania; gdy losowanie pozytywne, zostawia obecną zmiane
                    return 1;
                }
                i_last = i;
                j_last = j;
                // zapisanie ostatniej "legalnej" zamiany
            }
            swap(trasa2[i],trasa1[j]);
        }
        
    }
    if (j_last != -1 && i_last != -1) {// jeśli nie wylosowało wcześniej trasy, ustala ostanią zapisaną trasę
        swap(trasa2[i_last],trasa1[j_last]);
        return 1;
    }
    return -1;
}


// zlicza czas/koszt całego rozwiązania
double sumaryczny_czas_tras(vector<vector<int>> trasy) {
    double sumaryczny_czas = 0;
    for (int i = 0; i < (int)trasy.size(); i++) {
        sumaryczny_czas = sumaryczny_czas + koszt_trasy(trasy[i]);
    }
    return sumaryczny_czas;   
}

// sprawdza, czy podane rozwiązanie jest  w liście tabu
bool czy_w_tabu(vector<wynik> tabu, wynik rozwiazanie) {
    for(int i = 0; i < (int)tabu.size(); i++) {
        if((tabu[i].wszystkie_trasy == rozwiazanie.wszystkie_trasy) &&
        (tabu[i].czas == rozwiazanie.czas) && 
        (tabu[i].ilosc_tras == rozwiazanie.ilosc_tras)) {
            return true;
        }
    }
    return false;
}

// generuje zestaw nowych pozmienianych rozwiązań ("sąsiadów") na bazie podanego
// 1 - wymienia klientów między dwoma losowymi trasami w rozwiązaniu
// 2 - w przelosowanych trasach wykonuje algorytm two-opt
vector<wynik> generuj_sasiedztwo(wynik najlepsze_rozwiazanie, int rozmiar_sasiedztwa) {
    vector<wynik> sasiedzi;

    for(int i = 0; i < rozmiar_sasiedztwa; i++) {
        vector<vector<int>> sasiad;
        sasiad = najlepsze_rozwiazanie.wszystkie_trasy;
        random_device rd;  
        mt19937 gen(rd()); 
        uniform_int_distribution<> dis(0, sasiad.size() - 1);   

        int randTrasa1 = dis(gen), randTrasa2 = dis(gen);
        wymien_klientow(sasiad[randTrasa1], sasiad[randTrasa2]); // wymiana klintów

        
        two_opt(sasiad[randTrasa1]); // two-opt
        two_opt(sasiad[randTrasa2]);
        
        wynik temp;
        temp.czas = sumaryczny_czas_tras(sasiad);
        temp.ilosc_tras = (int)sasiad.size();
        temp.wszystkie_trasy = sasiad;
        sasiedzi.push_back(temp); // dodaje nowe rozwiązanie do listy sąsiadów
    }
    return sasiedzi;
}

// algorytm tabu
wynik tabu(int maxSasiedztwo, int maxTabu, int wykonywanieSek, int cap) {
    wynik najlepsze_rozwiazanie = zachlanny(cap); //generujemy rozwiązanie poprzednio napisanym algorytmem zachłannym
    wynik potencjalne_rozwiazanie = najlepsze_rozwiazanie;
    vector<wynik> lista_tabu;
    lista_tabu.push_back(najlepsze_rozwiazanie);

    if (najlepsze_rozwiazanie.ilosc_tras == -1) return najlepsze_rozwiazanie;

    auto start = chrono::high_resolution_clock::now();

    while(1) {
        vector<wynik> sasiedztwo = generuj_sasiedztwo(potencjalne_rozwiazanie, maxSasiedztwo);

        potencjalne_rozwiazanie = sasiedztwo[0];
        // sprawdzanie czy potencjalne rozwiązania są już w tabu i czy są lepsze niż obecne
        for(int i = 0; i < (int)sasiedztwo.size(); i ++) {
            if(!czy_w_tabu(lista_tabu, sasiedztwo[i]) && sasiedztwo[i].czas < potencjalne_rozwiazanie.czas) {
                potencjalne_rozwiazanie = sasiedztwo[i];
            }
        }

        // przypisanie najlepszego potencjalnego rozwiązania jeśli jest lepsze od obecnego
        if(najlepsze_rozwiazanie.czas > potencjalne_rozwiazanie.czas) { 
            najlepsze_rozwiazanie = potencjalne_rozwiazanie;
        }

        //dopisuje obecne rozwiązanie do tabu + wyrzuca najstarsze rozwiązanie tabu gdy przekroczy ustalony limit długości
        lista_tabu.push_back(najlepsze_rozwiazanie);
        if(maxTabu < (int)lista_tabu.size()) {
            lista_tabu.erase(lista_tabu.begin());
        }

        auto end = chrono::high_resolution_clock::now();
        double limit = chrono::duration_cast<chrono::nanoseconds>(end - start).count();
        limit *= 1e-9;
        if(limit > wykonywanieSek - 3) { // przerwanie tabu po upłynięciu czasu, 3 sekundy zapasu
            break;
        }
    }
    return najlepsze_rozwiazanie;

}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        printf("polecenie:  ./a.out <plik wejsciowy> <plik wyjsciowy> <ilu klientow wczytac (-1 - wszyscy)>\n");
        return -1;
    }

    auto start = chrono::high_resolution_clock::now();

    ofstream plik_wyjsciowy;
    plik_wyjsciowy.open(argv[2], ofstream::out);
    int pojazdy[2];    // 0 - liczba_pojazdow, 1 - ladownosc;
    

    if (wczytanie_pliku(argv[1], pojazdy, stoi(argv[3])) == -1) {
        cout << "Nie udalo sie otworzyc pliku :c" << endl;
        return -1;
    }

    ladunek = pojazdy[1];
    klienci_kopia = klienci;
    
    // ograniczenia dla algorytmu tabu
    int maxSasiedztwo = 50; //wielkość generowanego sąsiedztwa
    int maxTabu = 100; // wielkość listy tabu
    int wykonywanieSek = 300; // czas ile algorytm ma się wykonywać podawany w sekundach
    wynik rozwiazanie = tabu(maxSasiedztwo, maxTabu, wykonywanieSek, pojazdy[1]);

    if (rozwiazanie.ilosc_tras == -1) {
        cout.precision(6);
        cout << -1 << endl;
        plik_wyjsciowy.precision(6);
        plik_wyjsciowy << -1 << endl;
        
    } else {
        cout.precision(6);
        cout << rozwiazanie.ilosc_tras << " " << fixed << rozwiazanie.czas << endl;
        plik_wyjsciowy.precision(6);
        plik_wyjsciowy << rozwiazanie.ilosc_tras << " " << fixed << rozwiazanie.czas << endl;
        for (int i = 0; i < rozwiazanie.ilosc_tras; i++) {
            for (int tr : rozwiazanie.wszystkie_trasy[i]) {
                cout << tr << " ";
                plik_wyjsciowy << tr << " ";
            }
            cout << endl;
            plik_wyjsciowy << endl;
        }
    }

    plik_wyjsciowy.close();

    auto end = chrono::high_resolution_clock::now();
    double czas_wykonywania_programu = chrono::duration_cast<chrono::nanoseconds>(end - start).count();
    czas_wykonywania_programu *= 1e-9;
    cout << "Czas wykonywania programu: " << czas_wykonywania_programu << setprecision(9) << "s." << endl;

    return 0;
}
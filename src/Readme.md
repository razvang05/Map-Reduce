Gheorghe Marius Razvan 334CA

Tema 1 Algoritmi Pareleli si Distribuiti

Functii implementate:

In "main" citesc argumentele pe care le primesc,apoi deschid fisierul care 
contine toate intrarile,fisier pe care il citesc linie cu linie,salvand
intr un vector numele fisierelor si punand intr o coada indicele fiecarui fisier.
Creez vectorul de threaduri,declar elementele de sincronizare folosite si le 
initializez,apoi parcurg un for unde se va initializa structura fiecarui thread
cu datele necesare ,apoi se va crea fiecare thread luand ca si argument aceasta
structura dar cel mai important argument,functia pe care trebuie sa o execute.
Dupa aceasta iteratie voi astepta toate threadurile sa si termine executia si 
voi dezaloca elementele de sincronizare folosite.

In "thread_function()"primesc ca argument strucura fiecarui thread de unde voi 
extrage id ul sau si voi compara sa vad daca id ul este mai mic decat nr_maperi
daca da, threadul curent va executa functia de mapper,altfel va executa functia 
de reducer.

In "mapper_function" primesc ca argument structura threadului curent de unde
imi voi extrage elementele necesare in executarea functiei.Voi folosi un 
while(true) atata timp cat am elemente in coada,adica fiecare thread mapper
va extrage din coada un indice si va citi fisierul cu indicele respctiv pe care
il va prelucra mai departe, algoritm care face ca paralelizarea sa fie optima
deoarece anumite threaduri pot prelucra fisiere de dimensiuni mai mici si sa
termine mai repede,dar aceste lucru se face doar prin sincronizare ,folosind un 
mutex pe care voi face lock si unlock ca sa nu creez un acces concurent al 
threadurilor la coada.Cand threadul deschide fisierul curent citeste cuvant cu 
cuvant si le transforma cu functia process_word care primeste un cuvant
si il transforma sa contina fix formatul cerut.Aceste cuvinte sunt puse
intr un <set> pentru fiecare fisier ca sa nu contina cuvinte duplicate.
Fiecare thread are propriul sau local map pentru a nu exista conflicte
unde va itera prin set-ul in care are cuvintele din fiecare fisier
si le va adauga aici in <local_map> impreuna cu indicele al carui fisier
apartine cuvantul respectiv.
Apoi fiecare thread va scrie local_map in structura partajata de toate 
thread-urile dar cu ajutorul unui mutex pentru a nu exista race conditions.
Bariera sincronizează toate thread-urile mapper, asigurându-se că toate 
thread-urile au terminat de procesat fișierele și au adăugat <local_map>
în <mapper_results>(un <vector> de <unorder_map>).Este important ca toate 
threadurile mapper să termine procesarea înainte ca thread-urile reducer să înceapă.

In "reducer_function" astept ca toate thread-urile sa ajunga la bariera
iar apoi cele reducer isi vor incepe executia.Fiecare thread 
isi va lua un start si end(ca in laborator)unde va prelucra cuvintele
dupa literele care i se aloca,adica am ales start si end in functie de numarul
de thread-uri si de numarul literelor din alfabet.Fiecare thread reducer va itera
prin fiecare <local_map> din <mapper_results>.Pentru fiecare cuvant
se verifica daca apartine in intervalul [start,end] ,daca da sunt
adaugate impreuna cu id-urile ,id uri care sunt puse intr un <set>
pentru a elimina duplicatele ,toate acestea fiind agregate intr un <unorder_map>
care apoi va fi convertit in vector<string,int> pentru a fi ordonat 
prima oara descrescator dupa nr de fisiere in care apare,apoi alfabetic.
Apoi fiecare thread reducer va scrie rezultatele in fisiere cu ajutorul
functiei write_to_file unde se va verifica daca cuvantul curent incepe
cu litera curenta(adica fisierul de scriere curent).
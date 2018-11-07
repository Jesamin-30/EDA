#ifndef RTREE_H
#define RTREE_H

#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>

#include <algorithm>
#include <functional>

using namespace std;

template<int ndim, int maxn, int minn>
class Ctree{
protected: 
    struct Cnodo;

public:
    Ctree();
    ~Ctree();
    
    bool insertar(const double a_min[ndim], const double a_max[ndim], const string& dato);
    void leer_fichero(string);
    void guardar_linea();
    void guardar_linea(Cnodo *);
    
protected:
    struct rectangulo{
        double m_min[ndim];
        double m_max[ndim];
    };

    struct rama{
        rectangulo m_rect;
        Cnodo* m_hijo;
        string m_dato;
    };

    struct Cnodo{ 
        bool nodo_interno(){ 
            return (m_nivel > 0); 
        }
        bool hoja(){ 
            return (m_nivel == 0); 
        }
        
        int m_count;
        int m_nivel;
        rama vector_ramas[maxn];
    };
    
    struct Cparticion{
        enum { not_taken = -1 };//cuando hay particon coloca a todos -1, que no fue almacenado

        int m_particion[maxn+1];
        int m_total;
        int m_minNodo;//numero minimo de cada nodo 
        int m_count[2];
        rectangulo m_cover[2];//area que cubre a datos
        double m_area[2];//area calculado

        rama m_bufferRamas[maxn+1];//guarda ramas, 
        int m_countRamas;
        rectangulo m_coverSplit;//nodos que se haran split
        double m_coverSplitArea;//area calculado
    };
 
    Cnodo* separar_memoria();
    void  inicializarRect(rectangulo*);
    bool  insertarRect(const rama&, Cnodo*, Cnodo**, int);
    rectangulo cubrir_nodo(Cnodo*);//saca minimo y maximo que cubre
    bool  add_branch(const rama*, Cnodo*, Cnodo**);//puede estar vacio o lleno 
    int   pick_branch(const rectangulo*, Cnodo*);//devuelvee indice de los nodos de menor area
    rectangulo combinarRect(const rectangulo*, const rectangulo*);//combine rectaagulos
    void  split_node(Cnodo*, const rama*, Cnodo**);
    void  buffer_rama(Cnodo*, const rama*, Cparticion*);//carga todos las ramas en buffer
    void  elegir_particion(Cparticion*, int);//usa ick seed y calcua nuevo nodo 
    void  load_nodes(Cnodo*, Cnodo*, Cparticion*);//como ya elegi nodo, coloco del buffer al nodo que se asigno
    void  inicializarParticion(Cparticion*, int, int);//con not taken le pone -1
    void  pick_seeds(Cparticion*);//elegir 2 mas lejano
    void  clasificar(int, int, Cparticion*);//elige si va a nodo uno o 2 de la particion
    void  remove_all(Cnodo *);

    double calcular_area(rectangulo* a_rect);

    Cnodo* m_root;  
};

template<int ndim, int maxn, int minn>
Ctree<ndim, maxn, minn>::Ctree(){    
    m_root = separar_memoria();
    m_root->m_nivel = 0;
}

template<int ndim, int maxn, int minn>
Ctree<ndim, maxn, minn>::~Ctree(){    
    remove_all(m_root);
}

template<int ndim, int maxn, int minn>
void Ctree<ndim, maxn, minn>::remove_all(Cnodo *_node){
        if(_node->hoja()) return;
        for(int i=0; i<_node->m_count; i++){                
                guardar_linea(_node->vector_ramas[i].m_hijo);
        }
        delete _node;
}

template<int ndim, int maxn, int minn>
typename Ctree<ndim, maxn, minn>::Cnodo* Ctree<ndim, maxn, minn>::separar_memoria(){
    Cnodo* newNode;
    newNode = new Cnodo;
    newNode->m_count=0;
    newNode->m_nivel=-1;
    return newNode;
}

template<int ndim, int maxn, int minn>
void Ctree<ndim, maxn, minn>::inicializarRect(rectangulo* a_rect){
    for(int index = 0; index < ndim; ++index){
        a_rect->m_min[index] = (double)0;
        a_rect->m_max[index] = (double)0;
    }
}

template<int ndim, int maxn, int minn>
bool Ctree<ndim, maxn, minn>::insertar(const double a_min[ndim], const double a_max[ndim], const string& dato){
    rama a_branch;
    a_branch.m_dato = dato;//todo nodo que etra es hoja
    a_branch.m_hijo = NULL;
    
    for(int axis=0; axis<ndim; ++axis){
        a_branch.m_rect.m_min[axis] = a_min[axis];
        a_branch.m_rect.m_max[axis] = a_max[axis];
    }
    
    int a_level = 0;

    Cnodo* newNode;//para hacer recorrido
    if(insertarRect(a_branch, m_root, &newNode, a_level)){//v->si raiz esta dividido
        Cnodo* newRoot = separar_memoria();  //crea nuevaraiz
        newRoot->m_nivel = (m_root)->m_nivel + 1;

        rama branch;
        branch.m_rect = cubrir_nodo(m_root); //se busca rectsngulo, que cubre       
        branch.m_hijo = m_root; //branch apunte a root
        add_branch(&branch, newRoot, NULL);

        branch.m_rect = cubrir_nodo(newNode);
        branch.m_hijo = newNode;

        add_branch(&branch, newRoot, NULL);

        m_root = newRoot;        
        return true;
    }
    return false;
}

template<int ndim, int maxn, int minn> //es recursivo, va al ultuimo para hacer split
bool Ctree<ndim, maxn, minn>::insertarRect(const rama& a_branch, Cnodo* a_node, Cnodo** a_newNode, int a_level){
    if(a_node->m_nivel > a_level){
        Cnodo* otherNode;

        int index = pick_branch(&a_branch.m_rect, a_node);//elige indice para gacer mejor el area

        bool childWasSplit = insertarRect(a_branch, a_node->vector_ramas[index].m_hijo, &otherNode, a_level);
        if (!childWasSplit){            //si no hubo split sereajusta
            a_node->vector_ramas[index].m_rect = combinarRect(&a_branch.m_rect, &(a_node->vector_ramas[index].m_rect));
            return false;
        }
        else{//adjust tree
            a_node->vector_ramas[index].m_rect = cubrir_nodo(a_node->vector_ramas[index].m_hijo);//branch sube una capa
            rama branch;//sera añadido 
            branch.m_hijo = otherNode;
            branch.m_rect = cubrir_nodo(otherNode);
            return add_branch(&branch, a_node, a_newNode);//reajusta
            //avisa a capa artiba que hubo split
        }
    }
    else if(a_node->m_nivel == a_level){
        bool t = add_branch(&a_branch, a_node, a_newNode);//v si hubo split o f si no hubo
        return t;
    }
    else{
        return false;
    }
}

template<int ndim, int maxn, int minn>
typename Ctree<ndim, maxn, minn>::rectangulo Ctree<ndim, maxn, minn>::cubrir_nodo(Cnodo* a_node){
    rectangulo rect = a_node->vector_ramas[0].m_rect;
    for(int index = 1; index < a_node->m_count; ++index){
         rect = combinarRect(&rect, &(a_node->vector_ramas[index].m_rect));
    }
    
    return rect;
}

template<int ndim, int maxn, int minn>
bool Ctree<ndim, maxn, minn>::add_branch(const rama* a_branch, Cnodo* a_node, Cnodo** a_newNode){
    if(a_node->m_count < maxn){ 
        a_node->vector_ramas[a_node->m_count] = *a_branch;
        ++a_node->m_count;
        return false;
    }
    else{
        split_node(a_node, a_branch, a_newNode);
        return true;
    }    
}

template<int ndim, int maxn, int minn>
int Ctree<ndim, maxn, minn>::pick_branch(const rectangulo* a_rect, Cnodo* a_node){
    assert(a_rect && a_node);
    bool firstTime = true;
    double increase;
    double bestIncr = (double)-1;
    double area;
    double bestArea;
    int best = 0;
    rectangulo tempRect;

    for(int index=0; index < a_node->m_count; ++index){
        rectangulo* curRect = &a_node->vector_ramas[index].m_rect;
        area = calcular_area(curRect);
        tempRect = combinarRect(a_rect, curRect);
        increase = calcular_area(&tempRect) - area;

        if((increase < bestIncr) || firstTime){
            best = index;
            bestArea = area;
            bestIncr = increase;
            firstTime = false;
        }
        else if((increase == bestIncr) && (area < bestArea)){
            best = index;
            bestArea = area;
            bestIncr = increase;
        }
    }
    return best;
}//tiene mejor crecimiento de area

template<int ndim, int maxn, int minn>
typename Ctree<ndim, maxn, minn>::rectangulo Ctree<ndim, maxn, minn>::combinarRect(const rectangulo* a_rectA, const rectangulo* a_rectB){
    rectangulo newRect;

    for(int index = 0; index < ndim; ++index){
        newRect.m_min[index] = min(a_rectA->m_min[index], a_rectB->m_min[index]);
        newRect.m_max[index] = max(a_rectA->m_max[index], a_rectB->m_max[index]);
    }

    return newRect;
}

template<int ndim, int maxn, int minn>
void Ctree<ndim, maxn, minn>::split_node(Cnodo* a_node, const rama* a_branch, Cnodo** a_newNode){
    Cparticion localVars;
    Cparticion* parVars = &localVars;

    buffer_rama(a_node, a_branch, parVars);//almaceno branch al buffer de particion y luurgo le paso a choose
    elegir_particion(parVars, minn);//elige 2 lejanos

    *a_newNode = separar_memoria();
    (*a_newNode)->m_nivel = a_node->m_nivel;

    a_node->m_count = 0;
    load_nodes(a_node, *a_newNode, parVars);//al nodo que creo le asigno lo que ke toca
}

template<int ndim, int maxn, int minn>
double Ctree<ndim, maxn, minn>::calcular_area(rectangulo* a_rect){
    double volume = (double)1;

    for(int index=0; index<ndim; ++index){
        volume *= a_rect->m_max[index] - a_rect->m_min[index];
    }

    return volume;
}

template<int ndim, int maxn, int minn>
void Ctree<ndim, maxn, minn>::buffer_rama(Cnodo* a_node, const rama* a_branch, Cparticion* a_parVars){
    for(int index=0; index < maxn; ++index){
        a_parVars->m_bufferRamas[index] = a_node->vector_ramas[index];
    }
    a_parVars->m_bufferRamas[maxn] = *a_branch;
    a_parVars->m_countRamas = maxn + 1;

    a_parVars->m_coverSplit = a_parVars->m_bufferRamas[0].m_rect;
    for(int index=1; index < maxn+1; ++index){
        a_parVars->m_coverSplit = combinarRect(&a_parVars->m_coverSplit, &a_parVars->m_bufferRamas[index].m_rect);
    }

    a_parVars->m_coverSplitArea = calcular_area(&a_parVars->m_coverSplit);

}

template<int ndim, int maxn, int minn>
void Ctree<ndim, maxn, minn>::elegir_particion(Cparticion* a_parVars, int a_minFill){
    double biggestDiff;
    int group, chosen = 0, betterGroup = 0;
    
    inicializarParticion(a_parVars, a_parVars->m_countRamas, a_minFill);    
    pick_seeds(a_parVars);

    while (((a_parVars->m_count[0] + a_parVars->m_count[1]) < a_parVars->m_total)
             && (a_parVars->m_count[0] < (a_parVars->m_total - a_parVars->m_minNodo))
             && (a_parVars->m_count[1] < (a_parVars->m_total - a_parVars->m_minNodo)))
    {
        biggestDiff = (double) -1;
        for(int index=0; index<a_parVars->m_total; ++index){
            if(Cparticion::not_taken == a_parVars->m_particion[index]){
                rectangulo* curRect = &a_parVars->m_bufferRamas[index].m_rect;
                rectangulo rect0 = combinarRect(curRect, &a_parVars->m_cover[0]);
                rectangulo rect1 = combinarRect(curRect, &a_parVars->m_cover[1]);
                double growth0 = calcular_area(&rect0) - a_parVars->m_area[0];
                double growth1 = calcular_area(&rect1) - a_parVars->m_area[1];
                double diff = growth1 - growth0;
                if(diff >= 0){
                    group = 0;
                }
                else{
                    group = 1;
                    diff = -diff;
                }

                if(diff > biggestDiff){
                    biggestDiff = diff;
                    chosen = index;
                    betterGroup = group;
                }
                else if((diff == biggestDiff) && (a_parVars->m_count[group] < a_parVars->m_count[betterGroup])){
                    chosen = index;
                    betterGroup = group;
                }
            }
        }
        clasificar(chosen, betterGroup, a_parVars);
    }

    if((a_parVars->m_count[0] + a_parVars->m_count[1]) < a_parVars->m_total){
        if(a_parVars->m_count[0] >= a_parVars->m_total - a_parVars->m_minNodo){
            group = 1;
        }
        else{
            group = 0;
        }

        for(int index=0; index<a_parVars->m_total; ++index){
            if(Cparticion::not_taken == a_parVars->m_particion[index]){
                clasificar(index, group, a_parVars);
            }
        }
    }
}

// Copy branches from the buffer into two nodes according to the partition.
template<int ndim, int maxn, int minn>
void Ctree<ndim, maxn, minn>::load_nodes(Cnodo* a_nodeA, Cnodo* a_nodeB, Cparticion* a_parVars){
    for(int index=0; index < a_parVars->m_total; ++index){
        int targetNodeIndex = a_parVars->m_particion[index];
        Cnodo* targetNodes[] = {a_nodeA, a_nodeB};

        add_branch(&a_parVars->m_bufferRamas[index], targetNodes[targetNodeIndex], NULL);
    }
}

template<int ndim, int maxn, int minn>
void Ctree<ndim, maxn, minn>::inicializarParticion(Cparticion* a_parVars, int a_maxRects, int a_minFill){
    a_parVars->m_count[0] = a_parVars->m_count[1] = 0;
    a_parVars->m_area[0] = a_parVars->m_area[1] = (double)0;
    a_parVars->m_total = a_maxRects;
    a_parVars->m_minNodo = a_minFill;
    for(int index=0; index < a_maxRects; ++index){
        a_parVars->m_particion[index] = Cparticion::not_taken;
    }
}

template<int ndim, int maxn, int minn> //CUADRATICO
void Ctree<ndim, maxn, minn>::pick_seeds(Cparticion* a_parVars){
    int seed0 = 0, seed1 = 0;
    double worst, waste;
    double area[maxn+1];

    for(int index=0; index<a_parVars->m_total; ++index){
        area[index] = calcular_area(&a_parVars->m_bufferRamas[index].m_rect);
    }

    worst = -a_parVars->m_coverSplitArea - 1;
    for(int indexA=0; indexA < a_parVars->m_total-1; ++indexA){
        for(int indexB = indexA+1; indexB < a_parVars->m_total; ++indexB){
            rectangulo oneRect = combinarRect(&a_parVars->m_bufferRamas[indexA].m_rect, &a_parVars->m_bufferRamas[indexB].m_rect);
            waste = calcular_area(&oneRect) - area[indexA] - area[indexB];
            if(waste > worst){
                worst = waste;
                seed0 = indexA;
                seed1 = indexB;
            }
        }
    }

    clasificar(seed0, 0, a_parVars);
    clasificar(seed1, 1, a_parVars);
}

// Put a branch in one of the groups.
template<int ndim, int maxn, int minn>
void Ctree<ndim, maxn, minn>::clasificar(int a_index, int a_group, Cparticion* a_parVars){    
    a_parVars->m_particion[a_index] = a_group;

    if (a_parVars->m_count[a_group] == 0){
        a_parVars->m_cover[a_group] = a_parVars->m_bufferRamas[a_index].m_rect;
    }
    else{
        a_parVars->m_cover[a_group] = combinarRect(&a_parVars->m_bufferRamas[a_index].m_rect, &a_parVars->m_cover[a_group]);
    }

    a_parVars->m_area[a_group] = calcular_area(&a_parVars->m_cover[a_group]);
    ++a_parVars->m_count[a_group];
}

template<int ndim, int maxn, int minn>
void Ctree<ndim, maxn, minn>::leer_fichero(string filename){
        ifstream file(filename);
        string lat,lon,nam,tmp;
        if(!file.is_open()){
                cout << "error! cvs wrong!!" << endl;
                return;
        }

        xmin = 500;
        xmax = -500;

        ymin = 500;
        ymax = -500;

        getline(file,tmp,'\n');         //RowName
        double _min[2], _max[2];
        while(file.good()){
                getline(file,nam,',');            //Country     
                getline(file,lat,',');            //Latitude
                getline(file,lon,'\n');         //Longitude

                //cout << lat << " " << lon << " " << nam << endl;
                _min[0] = stod(lat);
                _min[1] = stod(lon);

                if(_min[0]<xmin)        xmin = _min[0];
                if(_min[0]>xmax)        xmax = _min[0];

                if(_min[1]<ymin)        ymin = _min[1];
                if(_min[1]>ymax)        ymax = _min[1];

                _max[0] = _min[0]+0.000000001;
                _max[1] = _min[1]+0.000000001;

                vpoints.push_back(_min[0]);
                vpoints.push_back(_min[1]);
                insertar(_min, _max, nam);
        }
        file.close();
}

template<int ndim, int maxn, int minn>
void Ctree<ndim, maxn, minn>::guardar_linea(){
        guardar_linea(this->m_root);
}

template<int ndim, int maxn, int minn>
void Ctree<ndim, maxn, minn>::guardar_linea(Cnodo *_node){
        if(_node->hoja()) return;
        for(int i=0; i<_node->m_count; i++){
                //cout << _node->vector_ramas[i].m_rect << endl;
                vlines.push_back(_node->vector_ramas[i].m_rect.m_min[0]);
                vlines.push_back(_node->vector_ramas[i].m_rect.m_min[1]);

                vlines.push_back(_node->vector_ramas[i].m_rect.m_max[0]);
                vlines.push_back(_node->vector_ramas[i].m_rect.m_max[1]);
                guardar_linea(_node->vector_ramas[i].m_hijo);
        }
}

#endif
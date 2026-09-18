#include <string.h>
#include "trump.h"

void make_card(struct trump m_card[]){
int i,j;
char shape[4][4] = {"♣","◆","♥","♠"};
for(i = 0; i < 4; i++){
    for(j = i*13; j < i*13+13;j++){
        m_card[j].order=i;
        strcpy(m_card[j].shape, shape[i]);
        m_card[j].number=j%13+1;
        switch(m_card[j].number){
            case 1:
                m_card[j].number = 'A';
                break;
            case 11:
                m_card[j].number = 'J';
                break;
            case 12:
                m_card[j].number = 'Q';
                break;
            case 13:
                m_card[j].number = 'K';
                break;
        }
    }
}
}
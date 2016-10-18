#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

#define MAX_STRING 512

struct node {
  char key[MAX_STRING];
  char value[MAX_STRING];
  struct node *next;
};

void Help(const char *const me);
int PushBack(struct node **const pList, struct node **const pTail, char *line);
void Destroy(struct node **const pList);
int ReadLineCopy(FILE *const pFile_in, char *const szLine_inout, const size_t cSize_in, size_t *const pcchLength_out);
int ConcatCopy(size_t *const plength, const size_t bufsize, char *const buffer, const size_t listcount, ...);

int main(int argc, char *argv[]) {
  if (argc < 3 || argc > 4)
    Help(argv[0]);

  FILE *pFile = fopen(argv[1], "r");
  if (!pFile && argc == 3)
    Help(argv[0]);

  struct node *list = NULL,
              *tail = NULL;
  char line[MAX_STRING] = {0};
  int found = 0;
  int deletekey = 0;

  if (argc == 3) {
    while (ReadLineCopy(pFile, line, MAX_STRING, NULL) == 0) {
      if (PushBack(&list, &tail, line) == 0 && strcmp(tail->key, argv[2]) == 0) {
        puts(tail->value);
        Destroy(&list);
        fclose(pFile);
        return 0;
      }
    }
    Destroy(&list);
    fclose(pFile);
    return 1;
  } else {
    deletekey = (strcmp(argv[3], "--delete") == 0);
    if (pFile) {
      struct node *before = tail;
      while (ReadLineCopy(pFile, line, MAX_STRING, NULL) == 0) {
        if (PushBack(&list, &tail, line) == 0 && strcmp(tail->key, argv[2]) == 0) {
          found = 1;
          if (deletekey) {
            if (list == tail)
              Destroy(&list);
            else {
              Destroy(&tail);
              tail = before;
              tail->next = NULL;
            }
          } else {
            strncpy(tail->value, argv[3], MAX_STRING);
            (tail->value)[MAX_STRING - 1] = 0;
          }
        }
        before = tail;
      }
      fclose(pFile);
    }
  }
 
  if (!found && deletekey) {
    Destroy(&list);
    return 1;
  } else if (!found) {
    if (ConcatCopy(NULL, MAX_STRING, line, 3, argv[2], "=", argv[3]) != 0 || PushBack(&list, &tail, line) != 0) {
      Destroy(&list);
      return 1;
    }
  }
 
  if (!(pFile = fopen(argv[1], "w"))) {
    Destroy(&list);
    return 1;
  }
 
  for (const struct node *pElement = list; pElement; pElement = pElement->next) {
    if (*(pElement->key) == 0)
      fputs("\n", pFile);
    else
      fprintf(pFile, "%s%c%s\n", pElement->key, '=', pElement->value);
  }
 
  Destroy(&list);
  fclose(pFile);
  return 0;
}
 
void Help(const char *const me) {
  fprintf(stderr,
          "%s filename key [value | --delete]\n\n"
          "  filename  name of the config file\n"
          "  key       key name of the value\n"
          "  value     value for the key (optional)\n"
          "            if specified the key=value pair will be updated or created\n"
          "            if omitted the value for the specified key will be printed\n"
          "  --delete  option to remove the specified key (optional)\n\n",
          me);
 
  exit(1);
}
 
int PushBack(struct node **const pList, struct node **const pTail, char *line) {
  struct node *pNew = NULL;
 
  if (!(pNew = (struct node*)calloc(1, sizeof(struct node))))
    return 1;
 
  char *pCh = strtok(line, "=");
  if (pCh) {
    strcpy(pNew->key, pCh);
    if ((pCh = strtok(NULL, "")))
      strcpy(pNew->value, pCh);
  }
 
  if (*pList) {
    (*pTail)->next = pNew;
    *pTail = pNew;
  } else {
    *pList = pNew;
    *pTail = pNew;
  }
  return 0;
}

void Destroy(struct node **const pList) {
  struct node *pCurrentElement = NULL,
              *pTempElement = NULL;

  if (pList) {
    pCurrentElement = *pList;
    while (pCurrentElement) {
      pTempElement = pCurrentElement;
      pCurrentElement = pCurrentElement->next;
      free(pTempElement);
    }
    *pList = NULL;
  }
}

/** \brief  Zeile einer Datei oder Benutzereingabe als nullterminierten String lesen
*
*  Der Filestream wird solang gelesen, bis ein Zeilenumbruch eingelesen wurde,
*    das Ende des Streams oder die Puffergröße erreicht ist.
*  Der Zeilenumbruch ist nicht Bestandteil des Strings.
*
*  \param  pFile_in        Filepointer des zu lesenden Dateistreams (stdin für Tastatureingabe)
*                            erforderlich
*  \param  szLine_inout    Charpointer auf einen Puffer in den die eingelesene Zeichenfolge kopiert wird
*                          muss ausreichend Elemente für den einzulesenden String incl. Nullterminierung haben
*                            erforderlich
*  \param  cSize_in        Anzahl Elemente im Puffer
*                            erforderlich
*  \param  pcchLength_out  Pointer auf ein size_t, das die Länge der eingelesenen Zeichenfolge zugewiesen bekommt
*                            darf NULL sein
*
*  \return int             1 bei Fehler, sonst 0
*/
int ReadLineCopy(FILE *const pFile_in, char *const szLine_inout, const size_t cSize_in, size_t *const pcchLength_out) {
  size_t cLen = 0; // Stringlänge
  int ch = EOF; // ASCII Wert eines Zeichens
 
  if (pcchLength_out) // wenn ein Pointer für den Längenwert übergeben wurde
    *pcchLength_out = 0; // Wert im Fehlerfall

  if (!szLine_inout || !cSize_in) // falls kein Puffer übergeben wurde oder die Größe des Puffers mit 0 angegeben wurde
    return 1; // Fehler, raus hier

  *szLine_inout = 0; // Wert im Fehlerfall

  if (!pFile_in) // falls kein Filepointer übergeben wurde
    return 1; // Fehler, raus hier

  if (fgets(szLine_inout, cSize_in, pFile_in) == NULL) { // Zeile lesen. Im Fehlerfall ...
    clearerr(pFile_in); // EOF und Fehlerindikatoren löschen
    *szLine_inout = 0; // falls szLine_inout bereits geändert wurde, rückgängig machen
    return 1; // Fehler, raus hier
  }

  if (!(cLen = strlen(szLine_inout))) // Länge der Eingabe. Falls die Länge 0 ist ...
    return 1; // Fehler, raus hier

  if (szLine_inout[cLen - 1] != '\n') { // falls das letzte Zeichen kein Zeilenumbruch ist, war ungenügend Platz um den gesamten String einzulesen
    if (pcchLength_out) // wenn ein Pointer für den Längenwert übergeben wurde
      *pcchLength_out = cLen; // Länge zuweisen

    if ((ch = fgetc(pFile_in)) == '\n' || ch == EOF) { // Wenn das nächste Zeichen ein Zeilenumbruch oder das Streamende ist
      clearerr(pFile_in); // EOF und Fehlerindikatoren löschen
      return 0; // OK und raus
    }

    while (!ferror(pFile_in) && (ch = fgetc(pFile_in)) != '\n' && ch != EOF); // Eingabepuffer leeren
    clearerr(pFile_in); // EOF und Fehlerindikatoren löschen
    return 1; // Fehler, raus hier
  }

  szLine_inout[--cLen] = 0; // Länge dekrementieren, Zeilenumbruch durch Nullterminierung ersetzen
  if (pcchLength_out) // wenn ein Pointer für den Längenwert übergeben wurde
    *pcchLength_out = cLen; // Länge zuweisen

  return 0; // OK und raus
}

/** \brief  Einzelstrings in einem Puffer verketten
*
*  \param  plength    Pointer auf ein size_t, das die Länge des verketteten Strings zugewiesen bekommt (darf NULL sein)
*  \param  bufsize    Größe des Puffers
*  \param  buffer     Puffer in den die Einzelstrings der Liste kopiert werden
*  \param  listcount  Anzahl Einzelstrings in der Liste
*  \param  ...        Liste der Einzelstrings
*
*  \return int        1 bei Fehler, sonst 0
*/
int ConcatCopy(size_t *const plength, const size_t bufsize, char *const buffer, const size_t listcount, ...) {
  va_list vl; // Liste der variablen Parameter
  size_t i = 0, len = 0, rem = bufsize - 1; // Schleifenindex, Länge eines Einzelstrings, verbleibende Puffergröße
  char *str = NULL, *it = buffer; // Einzelstring, Ende des bisherigen verketteten Strings im Puffer

  if (plength)
    *plength = 0; // Wert im Fehlerfall

  if (bufsize && buffer)
    *buffer = 0; // Wert im Fehlerfall

  if (!bufsize || ! buffer || !listcount) // Plausibilitätsprüfung
    return 1;

  va_start(vl, listcount); // Parameterliste initialisieren

  for (; i < listcount; ++i) { // für jeden Einzelstring in der Liste
    str = va_arg(vl, char*); // Einzelstring zuweisen
     if ((len = strlen(str)) > rem) { // falls die Länge des Einzelstrings größer als die verbleibende Puffergröße ist
      *it = 0; // Nullterminierung setzen
      va_end(vl); // Liste freigeben
      return 1; // Fehler und raus
    }
    strncpy(it, str, len); // Einzelstring an das Ende des bisherigen Strings anhängen
    rem -= len; // verbleibende Puffergröße um die Einzelstringlänge vermindern
    it += len; // Pointer auf das bisherige Stringende setzen

    if (plength)
      *plength += len; // Gesamtlänge aufaddieren
  }

  *it = 0; // Nullterminierung setzen
  va_end(vl); // Liste freigeben

  return 0; // OK und beenden
}
 
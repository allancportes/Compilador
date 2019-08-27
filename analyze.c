/****************************************************/
/* File: analyze.c                                  */
/* Semantic analyzer implementation                 */
/* for the C- compiler                              */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#include "globals.h"
#include "symtab.h"
#include "analyze.h"
#include <stdio.h>

/* counter for variable memory locations */
static int location = 0;
char* escopo = "global";

void UpdateScope(TreeNode * t)
{
  //printf("atualiza Escopo\n");
  if (t->child[0] != NULL && t->child[0]->kind.exp == FunDeclK) escopo = t->child[0]->attr.name;
}
/* Procedure traverse is a generic recursive
 * syntax tree traversal routine:
 * it applies preProc in preorder and postProc
 * in postorder to tree pointed to by t
 */
static void traverse( TreeNode * t,
               void (* preProc) (TreeNode *),
               void (* postProc) (TreeNode *) )
{
  //printf("Traverse\n");
  if (t != NULL)
  { UpdateScope(t);
    preProc(t);
    { int i;
      for (i=0; i < MAXCHILDREN; i++)
        traverse(t->child[i],preProc,postProc);
    }
    if(t->child[0]!= NULL && t->child[0]->kind.exp == FunDeclK) escopo = "global";
    postProc(t);
    traverse(t->sibling,preProc,postProc);
  }
}
/* nullProc is a do-nothing procedure to
 * generate preorder-only or postorder-only
 * traversals from traverse
 */
static void nullProc(TreeNode * t)
{ if (t==NULL) return;
  else return;
}

/* Procedure insertNode inserts
 * identifiers stored in t into
 * the symbol table
 */
static void insertNode( TreeNode * t)
{
   //printf("name: %s, escopo: %s \n",t->attr.name,t->scope);
  switch (t->nodekind){
    case StmtK:
      if(t->kind.stmt == AssignK)
      {
        //  printf("AssignK\n");
          if (st_lookup(t->child[0]->attr.name) == -1){
          /* não encontrado na tabela, cariavel não declarada */
            fprintf(listing,"Erro: A variavel %s não foi declarada. [%d]\n", t->child[0]->attr.name, t->lineno);
            Error = TRUE;
          }
          else
          /* encontrada na tabela, verificar escopo e adicionar linha */
            st_insert(t->child[0]->attr.name,t->lineno,0,escopo,INTTYPE,VAR,t->vet);
          t->child[0]->add = 1;
      }
      break;
    case ExpK:
      switch(t->kind.exp )
      {
        case TypeK:
          if(t->child[0] != NULL){
            switch (t->child[0]->kind.exp)
            {
              case VarDeclK:
                if (st_lookup(t->attr.name) == -1)
                /* não encontrado na tabela, inserir*/
                  st_insert(t->child[0]->attr.name,t->lineno,location++, escopo,INTTYPE, VAR , t->vet);
                else
                /* encontrado na tabela, verificar escopo */
                  st_insert(t->child[0]->attr.name,t->lineno,0, escopo,INTTYPE, VAR, t->vet);
                break;
              case FunDeclK:
                if (st_lookup(t->attr.name) == -1){
                  st_insert(t->child[0]->attr.name,t->lineno,location++, "global",t->child[0]->type,FUN, t->vet);}
                else
                /* encontrado na tabela, verificar escopo */
                  fprintf(listing,"Erro: Multiplas declarações da função %s. [%d]\n", t->child[0]->attr.name, t->lineno);
                break;
              default:
                  break;
            }
          }
          break;
        case ParamK:
          st_insert(t->attr.name,t->lineno,location++,escopo,INTTYPE, VAR, t->vet);
          break;
        case VetorK:
        st_insert(t->attr.name,t->lineno,0,escopo,INTTYPE, VAR, t->vet);
        break;
        case IdK:
          if(t->add != 1){
            if (st_lookup(t->attr.name) == -1){
              fprintf(listing,"Erro: A variavel %s não foi declarada. [%d]\n", t->attr.name, t->lineno);
              Error = TRUE;
            }
            else {
              st_insert(t->attr.name,t->lineno,0,escopo,INTTYPE,FUN, t->vet);
            }
          }
          break;
        case AtivK:
          if (st_lookup(t->attr.name) == -1 && strcmp(t->attr.name, "input")!=0 && strcmp(t->attr.name, "output")!=0){
            fprintf(listing,"Erro: A função %s não foi declarada. [%d]\n", t->attr.name, t->lineno);
            st_insert(t->attr.name,t->lineno,0,escopo,NULLL,CALL, t->vet);
            Error = TRUE;
          }
          else {
            st_insert(t->attr.name,t->lineno,0,escopo,NULLL,CALL, t->vet);
          }
          break;
        default:
          break;
      }
  }
}

/* Function buildSymtab constructs the symbol
 * table by preorder traversal of the syntax tree
 */
void buildSymtab(TreeNode * syntaxTree)
{
  //printf("buildSymtab\n");
  // TreeNode* aux = syntaxTree;
  traverse(syntaxTree,insertNode,nullProc);
  busca_main();
  typeCheck(syntaxTree);
  fprintf(listing,"\nSymbol table:\n\n");
  printSymTab(listing);
  
}

static void typeError(TreeNode * t, char * message)
{ fprintf(listing,"Erro de tipo na linha %d: %s\n",t->lineno,message);
  Error = TRUE;
}

/* Procedure checkNode performs
 * type checking at a single tree node
 */
void checkNode(TreeNode * t)
{

  //  printf("checkNode\n");
  switch (t->nodekind)
  { case ExpK:
      switch (t->kind.exp)
      { case OpK:
          if (((t->child[0]->kind.exp == AtivK) &&( getFunType(t->child[0]->attr.name)) == VOIDTYPE) ||
              ((t->child[1]->kind.exp == AtivK) && (getFunType(t->child[1]->attr.name) == VOIDTYPE)))
                typeError(t->child[0],"Ativação de função do tipo void na expressão");
          break;

        default:
          break;
      }
      break;
    case StmtK:
      switch (t->kind.stmt)
      {
        case AssignK:
          //printf("%s=%s  type = %d",t->child[0]->attr.name,t->child[1]->attr.name, getFunType(t->child[1]->attr.name));
          if (t->child[1]->kind.exp == AtivK && getFunType(t->child[1]->attr.name) == VOIDTYPE)
            typeError(t->child[0],"Função com retorno void não pode ser atribuido a uma variavel");
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}

/* Procedure typeCheck performs type checking
 * by a postorder syntax tree traversal
 */
void typeCheck(TreeNode * syntaxTree)
{
  //printf("typeCheck\n");
  traverse(syntaxTree,checkNode, nullProc);
}

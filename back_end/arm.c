//
// Created by tom on 23-2-25.
//
#include "arm.h"

//ri
int regi=0;
//si
int regs=0;
bool imm_is_valid(unsigned int imm){
    int i;
    for (i = 0; i <= 30; i += 2) {
        unsigned int rotated = (imm >> i) | (imm << (32 - i));
        if (rotated <= 255 && (rotated & ~0xff) == 0)
            return true;
    }
    return false;
}
int get_value_offset(HashMap *hashMap,Value*value){
    offset *node= HashMapGet(hashMap, value);
    return node->offset_sp;
}

void give_param_str(HashMap*hashMap,Value*value,char *name,int *ri){
    if(!isImm(value)){
        if(!HashMapContain(hashMap, value)){
            ;
        }else{
            if((strcmp(name,value->name)>0)&& (strlen(name)>= strlen(value->name))){
//                    表示该参数为传递过来的参数
//                printf("namelen:%d value_namelen:%d\n", strlen(name), strlen(value->name));
                int x= get_value_offset(hashMap,value);
                printf("    str r%d,[sp,#%d]\n",(*ri)++,x);
                HashMapRemove(hashMap,value);
            }
        }
    }
    return;
}

int get_siezof_sp(HashMap*hashMap){
    return HashMapSize(hashMap);
}

int get_value_pdata_inspdata_true(Value*value){
    return value->pdata->instruction_pdata.true_goto_location;
}

int get_value_pdata_inspdata_false(Value*value){
    return value->pdata->instruction_pdata.false_goto_location;
}

void arm_translate_ins(InstNode *ins){
//    int x=1;
    InstNode *head;
    HashMap *hashMap;
    for(;ins!=NULL;ins=get_next_inst(ins)) {
//        printf("%d\n",x++);
        if(ins->inst->Opcode==FunBegin){
            head=ins;
//            在进入函数时将offset初始化好，以供load指令使用
            hashMap=offset_init(ins);
            regi=0;
            regs=0;
            ins= arm_trans_FunBegin(ins,hashMap);
            offset_free(hashMap);
            hashMap=NULL;

            hashMap= offset_init(ins);
            ins= get_next_inst(ins);
        }
        ins=_arm_translate_ins(ins,head,hashMap);
        if(ins->inst->Opcode==Return){
            offset_free(hashMap);
            hashMap=NULL;
        }
    }

    return;
}

InstNode * arm_trans_Add_Sub_Mul(InstNode *ins,HashMap*hashMap){
    char opcode[4];
    if(ins->inst->Opcode==Add){
        strcpy(opcode,"add");
    } else if(ins->inst->Opcode==Sub){
        strcpy(opcode,"sub");
    } else{
        strcpy(opcode,"mul");
    }
    Value *value0=&ins->inst->user.value;
    Value *value1=user_get_operand_use(&ins->inst->user,0)->Val;
    Value *value2=user_get_operand_use(&ins->inst->user,1)->Val;
    if(isImm(value1)&& isImm(value2)){
        if(isIntType(value1->VTy)&& isIntType(value2->VTy)){
            int x1=value1->pdata->var_pdata.iVal;
            int x2=value2->pdata->var_pdata.iVal;
            if(imm_is_valid(x1)&&(imm_is_valid(x2))){
                printf("    %s r0,#%d,#%d\n",opcode,x1,x2);
            }else if ((!imm_is_valid(x1))&&(imm_is_valid(x2))){
                char arr[12]="0x";
                sprintf(arr+2,"%0x",x1);
                printf("    ldr r1,=%s\n",arr);
                printf("    %s r0,r1,#%d\n",opcode,x2);
            } else if((imm_is_valid(x1))&&(!imm_is_valid(x2))){
                char arr[12]="0x";
                sprintf(arr+2,"%0x",x2);
                printf("    ldr r2,=%s\n",arr);
                printf("    %s r0,#%d,r2\n",opcode,x1);
            }else{
                char arr1[12]="0x";
                sprintf(arr1+2,"%0x",x1);
                char arr2[12]="0x";
                sprintf(arr2+2,"%0x",x2);
                printf("    ldr r1,=%s\n",arr1);
                printf("    ldr r2,=%s\n",arr2);
                printf("    %s r0,r1,r2\n",opcode);
            }
            if(isIntType(value0->VTy)){
                ;
            }
            else{
                if(get_next_inst(ins)->inst->Opcode==Store){
                    int x=get_value_offset(hashMap,user_get_operand_use(&ins->inst->user,1)->Val);
                    ins= get_next_inst(ins);
                    printf("    vcvt.s32.f32 s0,r0\n");
                    printf("    vstr s0,[sp,#%d]\n",x);
                }
            }

        }
        else if(isIntType(value1->VTy)&& isFloatType(value2->VTy)){
            int x1=value1->pdata->var_pdata.iVal;
            float x2=value2->pdata->var_pdata.fVal;
            if(imm_is_valid(x1)){
                printf("    mov r1,#%d\n",x1);
                printf("    vcvt.s32.f32 s1,r1\n");
                int *xx2=(int*)&x2;
                char arr2[12]="0x";
                sprintf(arr2+2,"%0x",*xx2);
                printf("    ldr r2,=%s\n",arr2);
                printf("    vcvt.s32.f32 s2,r2\n");
                printf("    v%s.s32.f32 s0,s1,s2\n",opcode);
            }else{
                char arr1[12]="0x";
                sprintf(arr1+2,"%0x",x1);
                printf("    ldr r1,=%s\n",arr1);
                printf("    vcvt.s32.f32 s1,r1\n");

                int *xx2=(int*)&x2;
                char arr2[12]="0x";
                sprintf(arr2+2,"%0x",*xx2);
                printf("    ldr r2,=%s\n",arr2);
                printf("    vcvt.s32.f32 s2,r2\n");

                printf("    v%s.s32.f32 s0,s1,s2\n",opcode);
            }
            if(isIntType(value0->VTy)){
                printf("    vcvt.s32.f32 r0,s0\n");
            }
            else{
                if(get_next_inst(ins)->inst->Opcode==Store){
                    int x=get_value_offset(hashMap,user_get_operand_use(&ins->inst->user,1)->Val);
                    ins= get_next_inst(ins);
                    printf("    vstr s0,[sp,#%d]\n",x);
                }
            }
        }
        else if(isFloatType(value1->VTy)&& isIntType(value2->VTy)){
            float x1=value1->pdata->var_pdata.fVal;
            int x2=value2->pdata->var_pdata.iVal;
            if(imm_is_valid(x2)){
                int *xx1=(int*)&x1;
                char arr1[12]="0x";
                sprintf(arr1+2,"%0x",*xx1);
                printf("    ldr r1,=%s\n",arr1);
                printf("    vcvt.s32.f32 s1,r1\n");

                printf("    mov r2,#%d\n",x2);
                printf("    vcvt.s32.f32 s2,r2\n");

                printf("    v%s.s32.f32 s0,s1,s2\n",opcode);
            }else{
                int *xx1=(int*)&x1;
                char arr1[12]="0x";
                sprintf(arr1+2,"%0x",*xx1);
                printf("    ldr r1,=%s\n",arr1);
                printf("    vcvt.s32.f32 s1,r1\n");

                char arr2[12]="0x";
                sprintf(arr2+2,"%0x",x2);
                printf("    ldr r2,=%s\n",arr2);
                printf("    vcvt.s32.f32 s2,r2\n");

                printf("    v%s.s32.f32 s0,s1,s2\n",opcode);
            }
            if(isIntType(value0->VTy)){
                printf("    vcvt.s32.f32 r0,s0\n");
            }
            else{
                if(get_next_inst(ins)->inst->Opcode==Store){
                    int x=get_value_offset(hashMap,user_get_operand_use(&ins->inst->user,1)->Val);
                    ins= get_next_inst(ins);
                    printf("    vstr s0,[sp,#%d]\n",x);
                }
            }
        }
        else if(isFloatType(value1->VTy)&& isFloatType(value2->VTy)){
            float x1=value1->pdata->var_pdata.fVal;
            float x2=value2->pdata->var_pdata.fVal;
            int *xx1=(int*)&x1;
            char arr1[12]="0x";
            sprintf(arr1+2,"%0x",*xx1);
            printf("    ldr r1,=%s\n",arr1);
            printf("    vcvt.s32.f32 s1,r1\n");

            int *xx2=(int*)&x2;
            char arr2[12]="0x";
            sprintf(arr2+2,"%0x",*xx2);
            printf("    ldr r2,=%s\n",arr2);
            printf("    vcvt.s32.f32 s2,r2\n");

            printf("    v%s.s32.f32 s0,s1,s2\n",opcode);
            if(isIntType(value0->VTy)){
                printf("    vcvt.s32.f32 r0,s0\n");
            }
            else{
                if(get_next_inst(ins)->inst->Opcode==Store){
                    int x=get_value_offset(hashMap,user_get_operand_use(&ins->inst->user,1)->Val);
                    ins= get_next_inst(ins);
                    printf("    vstr s0,[sp,#%d]\n",x);
                }
            }
        }
    }
    if(isImm(value1)&& !isImm(value2)){
        if(isIntType(value1->VTy)&& isIntType(value2->VTy)){
            int x1=value1->pdata->var_pdata.iVal;
            if(imm_is_valid(x1)){
                printf("    %s r0,#%d,r0\n",opcode,x1);
            }else{
                char arr1[12]="0x";
                sprintf(arr1+2,"%0x",x1);
                printf("    ldr r1,=%s\n",arr1);
                printf("    %s r0,r1,r0\n",opcode);
            }
            if(isIntType(value0->VTy)){
                ;
            }
            else{
                if(get_next_inst(ins)->inst->Opcode==Store){
                    int x=get_value_offset(hashMap,user_get_operand_use(&ins->inst->user,1)->Val);
                    ins= get_next_inst(ins);
                    printf("    vcvt.s32.f32 s0,r0\n");
                    printf("    vstr s0,[sp,#%d]\n",x);
                }
            }

        }
        else if(isIntType(value1->VTy)&& isFloatType(value2->VTy)){
            int x1=value1->pdata->var_pdata.iVal;
            if(imm_is_valid(x1)){
                printf("    mov r1,#%d\n",x1);
                printf("    vcvt.s32.f32 s1,r1\n");
                printf("    v%s.s32.f32 s0,s1,s0\n",opcode);
            }else{
                char arr1[12]="0x";
                sprintf(arr1+2,"%0x",x1);
                printf("    ldr r1,=%s\n",arr1);
                printf("    vcvt.s32.f32 s1,r1\n");

                printf("    v%s.s32.f32 s0,s1,s0\n",opcode);
            }
            if(isIntType(value0->VTy)){
                printf("    vcvt.s32.f32 r0,s0\n");
            }
            else{
                if(get_next_inst(ins)->inst->Opcode==Store){
                    int x=get_value_offset(hashMap,user_get_operand_use(&ins->inst->user,1)->Val);
                    ins= get_next_inst(ins);
                    printf("    vstr s0,[sp,#%d]\n",x);
                }
            }
        }
        else if(isFloatType(value1->VTy)&& isIntType(value2->VTy)){
            float x1=value1->pdata->var_pdata.fVal;
            int *xx1=(int*)&x1;
            char arr1[12]="0x";
            sprintf(arr1+2,"%0x",*xx1);
            printf("    ldr r1,=%s\n",arr1);
            printf("    vcvt.s32.f32 s1,r1\n");

            printf("    vcvt.s32.f32 s0,r0\n");

            printf("    v%s.s32.f32 s0,s1,s0\n",opcode);
            if(isIntType(value0->VTy)){
                printf("    vcvt.s32.f32 r0,s0\n");
            }
            else{
                if(get_next_inst(ins)->inst->Opcode==Store){
                    int x=get_value_offset(hashMap,user_get_operand_use(&ins->inst->user,1)->Val);
                    ins= get_next_inst(ins);
                    printf("    vstr s0,[sp,#%d]\n",x);
                }
            }
        }
        else if(isFloatType(value1->VTy)&& isFloatType(value2->VTy)){
            float x1=value1->pdata->var_pdata.fVal;
            int *xx1=(int*)&x1;
            char arr1[12]="0x";
            sprintf(arr1+2,"%0x",*xx1);
            printf("    ldr r1,=%s\n",arr1);
            printf("    vcvt.s32.f32 s1,r1\n");

            printf("    v%s.s32.f32 s0,s1,s0\n",opcode);
            if(isIntType(value0->VTy)){
                printf("    vcvt.s32.f32 r0,s0\n");
            }
            else{
                if(get_next_inst(ins)->inst->Opcode==Store){
                    int x=get_value_offset(hashMap,user_get_operand_use(&ins->inst->user,1)->Val);
                    ins= get_next_inst(ins);
                    printf("    vstr s0,[sp,#%d]\n",x);
                }
            }
        }
    }
    if(!isImm(value1)&& isImm(value2)){
        if(isIntType(value1->VTy)&& isIntType(value2->VTy)){
            int x2=value2->pdata->var_pdata.iVal;
            if((imm_is_valid(x2))){
                printf("    %s r0,r0,#%d\n",opcode,x2);
            }else{
                char arr2[12]="0x";
                sprintf(arr2+2,"%0x",x2);
                printf("    ldr r2,=%s\n",arr2);
                printf("    %s r0,r0,r2\n",opcode);
            }
            if(isIntType(value0->VTy)){
                ;
            }
            else{
                if(get_next_inst(ins)->inst->Opcode==Store){
                    int x=get_value_offset(hashMap,user_get_operand_use(&ins->inst->user,1)->Val);
                    ins= get_next_inst(ins);
                    printf("    vcvt.s32.f32 s0,r0\n");
                    printf("    vstr s0,[sp,#%d]\n",x);
                }
            }

        }
        else if(isIntType(value1->VTy)&& isFloatType(value2->VTy)){
            float x2=value2->pdata->var_pdata.fVal;
            int *xx2=(int*)&x2;
            char arr2[12]="0x";
            sprintf(arr2+2,"%0x",*xx2);
            printf("    ldr r2,=%s\n",arr2);
            printf("    vcvt.s32.f32 s2,r2\n");

            printf("    v%s.s32.f32 s0,s0,s2\n",opcode);
            if(isIntType(value0->VTy)){
                printf("    vcvt.s32.f32 r0,s0\n");
            }
            else{
                if(get_next_inst(ins)->inst->Opcode==Store){
                    int x=get_value_offset(hashMap,user_get_operand_use(&ins->inst->user,1)->Val);
                    ins= get_next_inst(ins);
                    printf("    vstr s0,[sp,#%d]\n",x);
                }
            }
        }
        else if(isFloatType(value1->VTy)&& isIntType(value2->VTy)){
            int x2=value2->pdata->var_pdata.iVal;
            if(imm_is_valid(x2)){
                printf("    mov r2,#%d\n",x2);
                printf("    vcvt.s32.f32 s2,r2\n");

                printf("    v%s.s32.f32 s0,s0,s2\n",opcode);
            }else{
                char arr2[12]="0x";
                sprintf(arr2+2,"%0x",x2);
                printf("    ldr r2,=%s\n",arr2);
                printf("    vcvt.s32.f32 s2,r2\n");

                printf("    v%s.s32.f32 s0,s0,s2\n",opcode);
            }
            if(isIntType(value0->VTy)){
                printf("    vcvt.s32.f32 r0,s0\n");
            }
            else{
                if(get_next_inst(ins)->inst->Opcode==Store){
                    int x=get_value_offset(hashMap,user_get_operand_use(&ins->inst->user,1)->Val);
                    ins= get_next_inst(ins);
                    printf("    vstr s0,[sp,#%d]\n",x);
                }
            }
        }
        else if(isFloatType(value1->VTy)&& isFloatType(value2->VTy)){
            float x2=value2->pdata->var_pdata.fVal;
            int *xx2=(int*)&x2;
            char arr2[12]="0x";
            sprintf(arr2+2,"%0x",*xx2);
            printf("    ldr r2,=%s\n",arr2);
            printf("    vcvt.s32.f32 s2,r2\n");

            printf("    v%s.s32.f32 s0,s0,s2\n",opcode);
            if(isIntType(value0->VTy)){
                printf("    vcvt.s32.f32 r0,s0\n");
            }
            else{
                if(get_next_inst(ins)->inst->Opcode==Store){
                    int x=get_value_offset(hashMap,user_get_operand_use(&ins->inst->user,1)->Val);
                    ins= get_next_inst(ins);
                    printf("    vstr s0,[sp,#%d]\n",x);
                }
            }
        }
    }
    if(!isImm(value1)&& !isImm(value2)){
        if(isIntType(value1->VTy)&& isIntType(value2->VTy)){
            printf("    %s r0,r0,r1\n",opcode);
            if(isIntType(value0->VTy)){
                ;
            }
            else{
                if(get_next_inst(ins)->inst->Opcode==Store){
                    int x=get_value_offset(hashMap,user_get_operand_use(&ins->inst->user,1)->Val);
                    ins= get_next_inst(ins);
                    printf("    vcvt.s32.f32 s0,r0\n");
                    printf("    vstr s0,[sp,#%d]\n",x);
                }
            }

        }
        else if(isIntType(value1->VTy)&& isFloatType(value2->VTy)){
            printf("    vcvt.s32.f32 s1,r0\n");
            printf("    v%s.s32.f32 s0,s1,s0\n",opcode);
            if(isIntType(value0->VTy)){
                printf("    vcvt.s32.f32 r0,s0\n");
            }
            else{
                if(get_next_inst(ins)->inst->Opcode==Store){
                    int x=get_value_offset(hashMap,user_get_operand_use(&ins->inst->user,1)->Val);
                    ins= get_next_inst(ins);
                    printf("    vstr s0,[sp,#%d]\n",x);
                }
            }
        }
        else if(isFloatType(value1->VTy)&& isIntType(value2->VTy)){
            printf("    vcvt.s32.f32 s1,r0\n");
            printf("    v%s.s32.f32 s0,s0,s1\n",opcode);
            if(isIntType(value0->VTy)){
                printf("    vcvt.s32.f32 r0,s0\n");
            }
            else{
                if(get_next_inst(ins)->inst->Opcode==Store){
                    int x=get_value_offset(hashMap,user_get_operand_use(&ins->inst->user,1)->Val);
                    ins= get_next_inst(ins);
                    printf("    vstr s0,[sp,#%d]\n",x);
                }
            }
        }
        else if(isFloatType(value1->VTy)&& isFloatType(value2->VTy)){
            printf("    v%s.s32.f32 s0,s0,s1\n",opcode);
            if(isIntType(value0->VTy)){
                printf("    vcvt.s32.f32 r0,s0\n");
            }
            else{
                if(get_next_inst(ins)->inst->Opcode==Store){
                    int x=get_value_offset(hashMap,user_get_operand_use(&ins->inst->user,1)->Val);
                    ins= get_next_inst(ins);
                    printf("    vstr s0,[sp,#%d]\n",x);
                }
            }
        }
    }

    return  ins;

}

//InstNode * arm_trans_Sub(InstNode *ins,HashMap*hashMap){
//
//    Value *value0=&ins->inst->user.value;
//    Value *value1=user_get_operand_use(&ins->inst->user,0)->Val;
//    Value *value2=user_get_operand_use(&ins->inst->user,1)->Val;
//    if(isImm(value1)&& isImm(value2)){
//        if(isIntType(value1->VTy)&& isIntType(value2->VTy)){
//            int x1=value1->pdata->var_pdata.iVal;
//            int x2=value2->pdata->var_pdata.iVal;
//            if(imm_is_valid(x1)&&(imm_is_valid(x2))){
//                printf("    sub r0,#%d,#%d\n",x1,x2);
//            }else if ((!imm_is_valid(x1))&&(imm_is_valid(x2))){
//                char arr[12]="0x";
//                sprintf(arr+2,"%0x",x1);
//                printf("    ldr r1,=%s\n",arr);
//                printf("    sub r0,r1,#%d\n",x2);
//            } else if((imm_is_valid(x1))&&(!imm_is_valid(x2))){
//                char arr[12]="0x";
//                sprintf(arr+2,"%0x",x2);
//                printf("    ldr r2,=%s\n",arr);
//                printf("    sub r0,#%d,r2\n",x1);
//            }else{
//                char arr1[12]="0x";
//                sprintf(arr1+2,"%0x",x1);
//                char arr2[12]="0x";
//                sprintf(arr2+2,"%0x",x2);
//                printf("    ldr r1,=%s\n",arr1);
//                printf("    ldr r2,=%s\n",arr2);
//                printf("    sub r0,r1,r2\n");
//            }
//            if(isIntType(value0->VTy)){
//                ;
//            }
//            else{
//                if(get_next_inst(ins)->inst->Opcode==Store){
//                    int x=get_value_offset(hashMap,user_get_operand_use(&ins->inst->user,1)->Val);
//                    ins= get_next_inst(ins);
//                    printf("    vcvt.s32.f32 s0,r0\n");
//                    printf("    vstr s0,[sp,#%d]\n",x);
//                }
//            }
//
//        }
//        else if(isIntType(value1->VTy)&& isFloatType(value2->VTy)){
//            int x1=value1->pdata->var_pdata.iVal;
//            float x2=value2->pdata->var_pdata.fVal;
//            if(imm_is_valid(x1)){
//                printf("    mov r1,#%d\n",x1);
//                printf("    vcvt.s32.f32 s1,r1\n");
//                int *xx2=(int*)&x2;
//                char arr2[12]="0x";
//                sprintf(arr2+2,"%0x",*xx2);
//                printf("    ldr r2,=%s\n",arr2);
//                printf("    vcvt.s32.f32 s2,r2\n");
//                printf("    vsub.s32.f32 s0,s1,s2\n");
//            }else{
//                char arr1[12]="0x";
//                sprintf(arr1+2,"%0x",x1);
//                printf("    ldr r1,=%s\n",arr1);
//                printf("    vcvt.s32.f32 s1,r1\n");
//
//                int *xx2=(int*)&x2;
//                char arr2[12]="0x";
//                sprintf(arr2+2,"%0x",*xx2);
//                printf("    ldr r2,=%s\n",arr2);
//                printf("    vcvt.s32.f32 s2,r2\n");
//
//                printf("    vsub.s32.f32 s0,s1,s2\n");
//            }
//            if(isIntType(value0->VTy)){
//                printf("    vcvt.s32.f32 r0,s0\n");
//            }
//            else{
//                if(get_next_inst(ins)->inst->Opcode==Store){
//                    int x=get_value_offset(hashMap,user_get_operand_use(&ins->inst->user,1)->Val);
//                    ins= get_next_inst(ins);
//                    printf("    vstr s0,[sp,#%d]\n",x);
//                }
//            }
//        }
//        else if(isFloatType(value1->VTy)&& isIntType(value2->VTy)){
//            float x1=value1->pdata->var_pdata.fVal;
//            int x2=value2->pdata->var_pdata.iVal;
//            if(imm_is_valid(x2)){
//                int *xx1=(int*)&x1;
//                char arr1[12]="0x";
//                sprintf(arr1+2,"%0x",*xx1);
//                printf("    ldr r1,=%s\n",arr1);
//                printf("    vcvt.s32.f32 s1,r1\n");
//
//                printf("    mov r2,#%d\n",x2);
//                printf("    vcvt.s32.f32 s2,r2\n");
//
//                printf("    vsub.s32.f32 s0,s1,s2\n");
//            }else{
//                int *xx1=(int*)&x1;
//                char arr1[12]="0x";
//                sprintf(arr1+2,"%0x",*xx1);
//                printf("    ldr r1,=%s\n",arr1);
//                printf("    vcvt.s32.f32 s1,r1\n");
//
//                char arr2[12]="0x";
//                sprintf(arr2+2,"%0x",x2);
//                printf("    ldr r2,=%s\n",arr2);
//                printf("    vcvt.s32.f32 s2,r2\n");
//
//                printf("    vsub.s32.f32 s0,s1,s2\n");
//            }
//            if(isIntType(value0->VTy)){
//                printf("    vcvt.s32.f32 r0,s0\n");
//            }
//            else{
//                if(get_next_inst(ins)->inst->Opcode==Store){
//                    int x=get_value_offset(hashMap,user_get_operand_use(&ins->inst->user,1)->Val);
//                    ins= get_next_inst(ins);
//                    printf("    vstr s0,[sp,#%d]\n",x);
//                }
//            }
//        }
//        else if(isFloatType(value1->VTy)&& isFloatType(value2->VTy)){
//            float x1=value1->pdata->var_pdata.fVal;
//            float x2=value2->pdata->var_pdata.fVal;
//            int *xx1=(int*)&x1;
//            char arr1[12]="0x";
//            sprintf(arr1+2,"%0x",*xx1);
//            printf("    ldr r1,=%s\n",arr1);
//            printf("    vcvt.s32.f32 s1,r1\n");
//
//            int *xx2=(int*)&x2;
//            char arr2[12]="0x";
//            sprintf(arr2+2,"%0x",*xx2);
//            printf("    ldr r2,=%s\n",arr2);
//            printf("    vcvt.s32.f32 s2,r2\n");
//
//            printf("    vsub.s32.f32 s0,s1,s2\n");
//            if(isIntType(value0->VTy)){
//                printf("    vcvt.s32.f32 r0,s0\n");
//            }
//            else{
//                if(get_next_inst(ins)->inst->Opcode==Store){
//                    int x=get_value_offset(hashMap,user_get_operand_use(&ins->inst->user,1)->Val);
//                    ins= get_next_inst(ins);
//                    printf("    vstr s0,[sp,#%d]\n",x);
//                }
//            }
//        }
//    }
//    if(isImm(value1)&& !isImm(value2)){
//        if(isIntType(value1->VTy)&& isIntType(value2->VTy)){
//            int x1=value1->pdata->var_pdata.iVal;
//            if(imm_is_valid(x1)){
//                printf("    sub r0,#%d,r0\n",x1);
//            }else{
//                char arr1[12]="0x";
//                sprintf(arr1+2,"%0x",x1);
//                printf("    ldr r1,=%s\n",arr1);
//                printf("    sub r0,r1,r0\n");
//            }
//            if(isIntType(value0->VTy)){
//                ;
//            }
//            else{
//                if(get_next_inst(ins)->inst->Opcode==Store){
//                    int x=get_value_offset(hashMap,user_get_operand_use(&ins->inst->user,1)->Val);
//                    ins= get_next_inst(ins);
//                    printf("    vcvt.s32.f32 s0,r0\n");
//                    printf("    vstr s0,[sp,#%d]\n",x);
//                }
//            }
//
//        }
//        else if(isIntType(value1->VTy)&& isFloatType(value2->VTy)){
//            int x1=value1->pdata->var_pdata.iVal;
//            if(imm_is_valid(x1)){
//                printf("    mov r1,#%d\n",x1);
//                printf("    vcvt.s32.f32 s1,r1\n");
//                printf("    vsub.s32.f32 s0,s1,s0\n");
//            }else{
//                char arr1[12]="0x";
//                sprintf(arr1+2,"%0x",x1);
//                printf("    ldr r1,=%s\n",arr1);
//                printf("    vcvt.s32.f32 s1,r1\n");
//
//                printf("    vsub.s32.f32 s0,s1,s0\n");
//            }
//            if(isIntType(value0->VTy)){
//                printf("    vcvt.s32.f32 r0,s0\n");
//            }
//            else{
//                if(get_next_inst(ins)->inst->Opcode==Store){
//                    int x=get_value_offset(hashMap,user_get_operand_use(&ins->inst->user,1)->Val);
//                    ins= get_next_inst(ins);
//                    printf("    vstr s0,[sp,#%d]\n",x);
//                }
//            }
//        }
//        else if(isFloatType(value1->VTy)&& isIntType(value2->VTy)){
//            float x1=value1->pdata->var_pdata.fVal;
//            int *xx1=(int*)&x1;
//            char arr1[12]="0x";
//            sprintf(arr1+2,"%0x",*xx1);
//            printf("    ldr r1,=%s\n",arr1);
//            printf("    vcvt.s32.f32 s1,r1\n");
//
//            printf("    vcvt.s32.f32 s0,r0\n");
//
//            printf("    vsub.s32.f32 s0,s1,s0\n");
//            if(isIntType(value0->VTy)){
//                printf("    vcvt.s32.f32 r0,s0\n");
//            }
//            else{
//                if(get_next_inst(ins)->inst->Opcode==Store){
//                    int x=get_value_offset(hashMap,user_get_operand_use(&ins->inst->user,1)->Val);
//                    ins= get_next_inst(ins);
//                    printf("    vstr s0,[sp,#%d]\n",x);
//                }
//            }
//        }
//        else if(isFloatType(value1->VTy)&& isFloatType(value2->VTy)){
//            float x1=value1->pdata->var_pdata.fVal;
//            int *xx1=(int*)&x1;
//            char arr1[12]="0x";
//            sprintf(arr1+2,"%0x",*xx1);
//            printf("    ldr r1,=%s\n",arr1);
//            printf("    vcvt.s32.f32 s1,r1\n");
//
//            printf("    vsub.s32.f32 s0,s1,s0\n");
//            if(isIntType(value0->VTy)){
//                printf("    vcvt.s32.f32 r0,s0\n");
//            }
//            else{
//                if(get_next_inst(ins)->inst->Opcode==Store){
//                    int x=get_value_offset(hashMap,user_get_operand_use(&ins->inst->user,1)->Val);
//                    ins= get_next_inst(ins);
//                    printf("    vstr s0,[sp,#%d]\n",x);
//                }
//            }
//        }
//    }
//    if(!isImm(value1)&& isImm(value2)){
//        if(isIntType(value1->VTy)&& isIntType(value2->VTy)){
//            int x2=value2->pdata->var_pdata.iVal;
//            if((imm_is_valid(x2))){
//                printf("    sub r0,r0,#%d\n",x2);
//            }else{
//                char arr2[12]="0x";
//                sprintf(arr2+2,"%0x",x2);
//                printf("    ldr r2,=%s\n",arr2);
//                printf("    sub r0,r0,r2\n");
//            }
//            if(isIntType(value0->VTy)){
//                ;
//            }
//            else{
//                if(get_next_inst(ins)->inst->Opcode==Store){
//                    int x=get_value_offset(hashMap,user_get_operand_use(&ins->inst->user,1)->Val);
//                    ins= get_next_inst(ins);
//                    printf("    vcvt.s32.f32 s0,r0\n");
//                    printf("    vstr s0,[sp,#%d]\n",x);
//                }
//            }
//
//        }
//        else if(isIntType(value1->VTy)&& isFloatType(value2->VTy)){
//            float x2=value2->pdata->var_pdata.fVal;
//            int *xx2=(int*)&x2;
//            char arr2[12]="0x";
//            sprintf(arr2+2,"%0x",*xx2);
//            printf("    ldr r2,=%s\n",arr2);
//            printf("    vcvt.s32.f32 s2,r2\n");
//
//            printf("    vsub.s32.f32 s0,s0,s2\n");
//            if(isIntType(value0->VTy)){
//                printf("    vcvt.s32.f32 r0,s0\n");
//            }
//            else{
//                if(get_next_inst(ins)->inst->Opcode==Store){
//                    int x=get_value_offset(hashMap,user_get_operand_use(&ins->inst->user,1)->Val);
//                    ins= get_next_inst(ins);
//                    printf("    vstr s0,[sp,#%d]\n",x);
//                }
//            }
//        }
//        else if(isFloatType(value1->VTy)&& isIntType(value2->VTy)){
//            int x2=value2->pdata->var_pdata.iVal;
//            if(imm_is_valid(x2)){
//                printf("    mov r2,#%d\n",x2);
//                printf("    vcvt.s32.f32 s2,r2\n");
//
//                printf("    vsub.s32.f32 s0,s0,s2\n");
//            }else{
//                char arr2[12]="0x";
//                sprintf(arr2+2,"%0x",x2);
//                printf("    ldr r2,=%s\n",arr2);
//                printf("    vcvt.s32.f32 s2,r2\n");
//
//                printf("    vsub.s32.f32 s0,s0,s2\n");
//            }
//            if(isIntType(value0->VTy)){
//                printf("    vcvt.s32.f32 r0,s0\n");
//            }
//            else{
//                if(get_next_inst(ins)->inst->Opcode==Store){
//                    int x=get_value_offset(hashMap,user_get_operand_use(&ins->inst->user,1)->Val);
//                    ins= get_next_inst(ins);
//                    printf("    vstr s0,[sp,#%d]\n",x);
//                }
//            }
//        }
//        else if(isFloatType(value1->VTy)&& isFloatType(value2->VTy)){
//            float x2=value2->pdata->var_pdata.fVal;
//            int *xx2=(int*)&x2;
//            char arr2[12]="0x";
//            sprintf(arr2+2,"%0x",*xx2);
//            printf("    ldr r2,=%s\n",arr2);
//            printf("    vcvt.s32.f32 s2,r2\n");
//
//            printf("    vsub.s32.f32 s0,s0,s2\n");
//            if(isIntType(value0->VTy)){
//                printf("    vcvt.s32.f32 r0,s0\n");
//            }
//            else{
//                if(get_next_inst(ins)->inst->Opcode==Store){
//                    int x=get_value_offset(hashMap,user_get_operand_use(&ins->inst->user,1)->Val);
//                    ins= get_next_inst(ins);
//                    printf("    vstr s0,[sp,#%d]\n",x);
//                }
//            }
//        }
//    }
//    if(!isImm(value1)&& !isImm(value2)){
//        if(isIntType(value1->VTy)&& isIntType(value2->VTy)){
//            printf("    sub r0,r0,r1\n");
//            if(isIntType(value0->VTy)){
//                ;
//            }
//            else{
//                if(get_next_inst(ins)->inst->Opcode==Store){
//                    int x=get_value_offset(hashMap,user_get_operand_use(&ins->inst->user,1)->Val);
//                    ins= get_next_inst(ins);
//                    printf("    vcvt.s32.f32 s0,r0\n");
//                    printf("    vstr s0,[sp,#%d]\n",x);
//                }
//            }
//
//        }
//        else if(isIntType(value1->VTy)&& isFloatType(value2->VTy)){
//            printf("    vcvt.s32.f32 s1,r0\n");
//            printf("    vsub.s32.f32 s0,s1,s0\n");
//            if(isIntType(value0->VTy)){
//                printf("    vcvt.s32.f32 r0,s0\n");
//            }
//            else{
//                if(get_next_inst(ins)->inst->Opcode==Store){
//                    int x=get_value_offset(hashMap,user_get_operand_use(&ins->inst->user,1)->Val);
//                    ins= get_next_inst(ins);
//                    printf("    vstr s0,[sp,#%d]\n",x);
//                }
//            }
//        }
//        else if(isFloatType(value1->VTy)&& isIntType(value2->VTy)){
//            printf("    vcvt.s32.f32 s1,r0\n");
//            printf("    vsub.s32.f32 s0,s0,s1\n");
//            if(isIntType(value0->VTy)){
//                printf("    vcvt.s32.f32 r0,s0\n");
//            }
//            else{
//                if(get_next_inst(ins)->inst->Opcode==Store){
//                    int x=get_value_offset(hashMap,user_get_operand_use(&ins->inst->user,1)->Val);
//                    ins= get_next_inst(ins);
//                    printf("    vstr s0,[sp,#%d]\n",x);
//                }
//            }
//        }
//        else if(isFloatType(value1->VTy)&& isFloatType(value2->VTy)){
//            printf("    vsub.s32.f32 s0,s0,s1\n");
//            if(isIntType(value0->VTy)){
//                printf("    vcvt.s32.f32 r0,s0\n");
//            }
//            else{
//                if(get_next_inst(ins)->inst->Opcode==Store){
//                    int x=get_value_offset(hashMap,user_get_operand_use(&ins->inst->user,1)->Val);
//                    ins= get_next_inst(ins);
//                    printf("    vstr s0,[sp,#%d]\n",x);
//                }
//            }
//        }
//    }
//
//    return  ins;
//}
//
//InstNode * arm_trans_Mul(InstNode *ins,HashMap*hashMap){
//    Value *value0=&ins->inst->user.value;
//    Value *value1=user_get_operand_use(&ins->inst->user,0)->Val;
//    Value *value2=user_get_operand_use(&ins->inst->user,1)->Val;
//    if(isImm(value1)&& isImm(value2)){
//        if(isIntType(value1->VTy)&& isIntType(value2->VTy)){
//            int x1=value1->pdata->var_pdata.iVal;
//            int x2=value2->pdata->var_pdata.iVal;
//            if(imm_is_valid(x1)&&(imm_is_valid(x2))){
//                printf("    mul r0,#%d,#%d\n",x1,x2);
//            }else if ((!imm_is_valid(x1))&&(imm_is_valid(x2))){
//                char arr[12]="0x";
//                sprintf(arr+2,"%0x",x1);
//                printf("    ldr r1,=%s\n",arr);
//                printf("    mul r0,r1,#%d\n",x2);
//            } else if((imm_is_valid(x1))&&(!imm_is_valid(x2))){
//                char arr[12]="0x";
//                sprintf(arr+2,"%0x",x2);
//                printf("    ldr r2,=%s\n",arr);
//                printf("    mul r0,#%d,r2\n",x1);
//            }else{
//                char arr1[12]="0x";
//                sprintf(arr1+2,"%0x",x1);
//                char arr2[12]="0x";
//                sprintf(arr2+2,"%0x",x2);
//                printf("    ldr r1,=%s\n",arr1);
//                printf("    ldr r2,=%s\n",arr2);
//                printf("    mul r0,r1,r2\n");
//            }
//            if(isIntType(value0->VTy)){
//                ;
//            }
//            else{
//                if(get_next_inst(ins)->inst->Opcode==Store){
//                    int x=get_value_offset(hashMap,user_get_operand_use(&ins->inst->user,1)->Val);
//                    ins= get_next_inst(ins);
//                    printf("    vcvt.s32.f32 s0,r0\n");
//                    printf("    vstr s0,[sp,#%d]\n",x);
//                }
//            }
//
//        }
//        else if(isIntType(value1->VTy)&& isFloatType(value2->VTy)){
//            int x1=value1->pdata->var_pdata.iVal;
//            float x2=value2->pdata->var_pdata.fVal;
//            if(imm_is_valid(x1)){
//                printf("    mov r1,#%d\n",x1);
//                printf("    vcvt.s32.f32 s1,r1\n");
//                int *xx2=(int*)&x2;
//                char arr2[12]="0x";
//                sprintf(arr2+2,"%0x",*xx2);
//                printf("    ldr r2,=%s\n",arr2);
//                printf("    vcvt.s32.f32 s2,r2\n");
//                printf("    vmul.s32.f32 s0,s1,s2\n");
//            }else{
//                char arr1[12]="0x";
//                sprintf(arr1+2,"%0x",x1);
//                printf("    ldr r1,=%s\n",arr1);
//                printf("    vcvt.s32.f32 s1,r1\n");
//
//                int *xx2=(int*)&x2;
//                char arr2[12]="0x";
//                sprintf(arr2+2,"%0x",*xx2);
//                printf("    ldr r2,=%s\n",arr2);
//                printf("    vcvt.s32.f32 s2,r2\n");
//
//                printf("    vmul.s32.f32 s0,s1,s2\n");
//            }
//            if(isIntType(value0->VTy)){
//                printf("    vcvt.s32.f32 r0,s0\n");
//            }
//            else{
//                if(get_next_inst(ins)->inst->Opcode==Store){
//                    int x=get_value_offset(hashMap,user_get_operand_use(&ins->inst->user,1)->Val);
//                    ins= get_next_inst(ins);
//                    printf("    vstr s0,[sp,#%d]\n",x);
//                }
//            }
//        }
//        else if(isFloatType(value1->VTy)&& isIntType(value2->VTy)){
//            float x1=value1->pdata->var_pdata.fVal;
//            int x2=value2->pdata->var_pdata.iVal;
//            if(imm_is_valid(x2)){
//                int *xx1=(int*)&x1;
//                char arr1[12]="0x";
//                sprintf(arr1+2,"%0x",*xx1);
//                printf("    ldr r1,=%s\n",arr1);
//                printf("    vcvt.s32.f32 s1,r1\n");
//
//                printf("    mov r2,#%d\n",x2);
//                printf("    vcvt.s32.f32 s2,r2\n");
//
//                printf("    vmul.s32.f32 s0,s1,s2\n");
//            }else{
//                int *xx1=(int*)&x1;
//                char arr1[12]="0x";
//                sprintf(arr1+2,"%0x",*xx1);
//                printf("    ldr r1,=%s\n",arr1);
//                printf("    vcvt.s32.f32 s1,r1\n");
//
//                char arr2[12]="0x";
//                sprintf(arr2+2,"%0x",x2);
//                printf("    ldr r2,=%s\n",arr2);
//                printf("    vcvt.s32.f32 s2,r2\n");
//
//                printf("    vmul.s32.f32 s0,s1,s2\n");
//            }
//            if(isIntType(value0->VTy)){
//                printf("    vcvt.s32.f32 r0,s0\n");
//            }
//            else{
//                if(get_next_inst(ins)->inst->Opcode==Store){
//                    int x=get_value_offset(hashMap,user_get_operand_use(&ins->inst->user,1)->Val);
//                    ins= get_next_inst(ins);
//                    printf("    vstr s0,[sp,#%d]\n",x);
//                }
//            }
//        }
//        else if(isFloatType(value1->VTy)&& isFloatType(value2->VTy)){
//            float x1=value1->pdata->var_pdata.fVal;
//            float x2=value2->pdata->var_pdata.fVal;
//            int *xx1=(int*)&x1;
//            char arr1[12]="0x";
//            sprintf(arr1+2,"%0x",*xx1);
//            printf("    ldr r1,=%s\n",arr1);
//            printf("    vcvt.s32.f32 s1,r1\n");
//
//            int *xx2=(int*)&x2;
//            char arr2[12]="0x";
//            sprintf(arr2+2,"%0x",*xx2);
//            printf("    ldr r2,=%s\n",arr2);
//            printf("    vcvt.s32.f32 s2,r2\n");
//
//            printf("    vmul.s32.f32 s0,s1,s2\n");
//            if(isIntType(value0->VTy)){
//                printf("    vcvt.s32.f32 r0,s0\n");
//            }
//            else{
//                if(get_next_inst(ins)->inst->Opcode==Store){
//                    int x=get_value_offset(hashMap,user_get_operand_use(&ins->inst->user,1)->Val);
//                    ins= get_next_inst(ins);
//                    printf("    vstr s0,[sp,#%d]\n",x);
//                }
//            }
//        }
//    }
//    if(isImm(value1)&& !isImm(value2)){
//        if(isIntType(value1->VTy)&& isIntType(value2->VTy)){
//            int x1=value1->pdata->var_pdata.iVal;
//            if(imm_is_valid(x1)){
//                printf("    mul r0,#%d,r0\n",x1);
//            }else{
//                char arr1[12]="0x";
//                sprintf(arr1+2,"%0x",x1);
//                printf("    ldr r1,=%s\n",arr1);
//                printf("    mul r0,r1,r0\n");
//            }
//            if(isIntType(value0->VTy)){
//                ;
//            }
//            else{
//                if(get_next_inst(ins)->inst->Opcode==Store){
//                    int x=get_value_offset(hashMap,user_get_operand_use(&ins->inst->user,1)->Val);
//                    ins= get_next_inst(ins);
//                    printf("    vcvt.s32.f32 s0,r0\n");
//                    printf("    vstr s0,[sp,#%d]\n",x);
//                }
//            }
//
//        }
//        else if(isIntType(value1->VTy)&& isFloatType(value2->VTy)){
//            int x1=value1->pdata->var_pdata.iVal;
//            if(imm_is_valid(x1)){
//                printf("    mov r1,#%d\n",x1);
//                printf("    vcvt.s32.f32 s1,r1\n");
//                printf("    vmul.s32.f32 s0,s1,s0\n");
//            }else{
//                char arr1[12]="0x";
//                sprintf(arr1+2,"%0x",x1);
//                printf("    ldr r1,=%s\n",arr1);
//                printf("    vcvt.s32.f32 s1,r1\n");
//
//                printf("    vmul.s32.f32 s0,s1,s0\n");
//            }
//            if(isIntType(value0->VTy)){
//                printf("    vcvt.s32.f32 r0,s0\n");
//            }
//            else{
//                if(get_next_inst(ins)->inst->Opcode==Store){
//                    int x=get_value_offset(hashMap,user_get_operand_use(&ins->inst->user,1)->Val);
//                    ins= get_next_inst(ins);
//                    printf("    vstr s0,[sp,#%d]\n",x);
//                }
//            }
//        }
//        else if(isFloatType(value1->VTy)&& isIntType(value2->VTy)){
//            float x1=value1->pdata->var_pdata.fVal;
//            int *xx1=(int*)&x1;
//            char arr1[12]="0x";
//            sprintf(arr1+2,"%0x",*xx1);
//            printf("    ldr r1,=%s\n",arr1);
//            printf("    vcvt.s32.f32 s1,r1\n");
//
//            printf("    vcvt.s32.f32 s0,r0\n");
//
//            printf("    vmul.s32.f32 s0,s1,s0\n");
//            if(isIntType(value0->VTy)){
//                printf("    vcvt.s32.f32 r0,s0\n");
//            }
//            else{
//                if(get_next_inst(ins)->inst->Opcode==Store){
//                    int x=get_value_offset(hashMap,user_get_operand_use(&ins->inst->user,1)->Val);
//                    ins= get_next_inst(ins);
//                    printf("    vstr s0,[sp,#%d]\n",x);
//                }
//            }
//        }
//        else if(isFloatType(value1->VTy)&& isFloatType(value2->VTy)){
//            float x1=value1->pdata->var_pdata.fVal;
//            int *xx1=(int*)&x1;
//            char arr1[12]="0x";
//            sprintf(arr1+2,"%0x",*xx1);
//            printf("    ldr r1,=%s\n",arr1);
//            printf("    vcvt.s32.f32 s1,r1\n");
//
//            printf("    vmul.s32.f32 s0,s1,s0\n");
//            if(isIntType(value0->VTy)){
//                printf("    vcvt.s32.f32 r0,s0\n");
//            }
//            else{
//                if(get_next_inst(ins)->inst->Opcode==Store){
//                    int x=get_value_offset(hashMap,user_get_operand_use(&ins->inst->user,1)->Val);
//                    ins= get_next_inst(ins);
//                    printf("    vstr s0,[sp,#%d]\n",x);
//                }
//            }
//        }
//    }
//    if(!isImm(value1)&& isImm(value2)){
//        if(isIntType(value1->VTy)&& isIntType(value2->VTy)){
//            int x2=value2->pdata->var_pdata.iVal;
//            if((imm_is_valid(x2))){
//                printf("    mul r0,r0,#%d\n",x2);
//            }else{
//                char arr2[12]="0x";
//                sprintf(arr2+2,"%0x",x2);
//                printf("    ldr r2,=%s\n",arr2);
//                printf("    mul r0,r0,r2\n");
//            }
//            if(isIntType(value0->VTy)){
//                ;
//            }
//            else{
//                if(get_next_inst(ins)->inst->Opcode==Store){
//                    int x=get_value_offset(hashMap,user_get_operand_use(&ins->inst->user,1)->Val);
//                    ins= get_next_inst(ins);
//                    printf("    vcvt.s32.f32 s0,r0\n");
//                    printf("    vstr s0,[sp,#%d]\n",x);
//                }
//            }
//
//        }
//        else if(isIntType(value1->VTy)&& isFloatType(value2->VTy)){
//            float x2=value2->pdata->var_pdata.fVal;
//            int *xx2=(int*)&x2;
//            char arr2[12]="0x";
//            sprintf(arr2+2,"%0x",*xx2);
//            printf("    ldr r2,=%s\n",arr2);
//            printf("    vcvt.s32.f32 s2,r2\n");
//
//            printf("    vmul.s32.f32 s0,s0,s2\n");
//            if(isIntType(value0->VTy)){
//                printf("    vcvt.s32.f32 r0,s0\n");
//            }
//            else{
//                if(get_next_inst(ins)->inst->Opcode==Store){
//                    int x=get_value_offset(hashMap,user_get_operand_use(&ins->inst->user,1)->Val);
//                    ins= get_next_inst(ins);
//                    printf("    vstr s0,[sp,#%d]\n",x);
//                }
//            }
//        }
//        else if(isFloatType(value1->VTy)&& isIntType(value2->VTy)){
//            int x2=value2->pdata->var_pdata.iVal;
//            if(imm_is_valid(x2)){
//                printf("    mov r2,#%d\n",x2);
//                printf("    vcvt.s32.f32 s2,r2\n");
//
//                printf("    vmul.s32.f32 s0,s0,s2\n");
//            }else{
//                char arr2[12]="0x";
//                sprintf(arr2+2,"%0x",x2);
//                printf("    ldr r2,=%s\n",arr2);
//                printf("    vcvt.s32.f32 s2,r2\n");
//
//                printf("    vmul.s32.f32 s0,s0,s2\n");
//            }
//            if(isIntType(value0->VTy)){
//                printf("    vcvt.s32.f32 r0,s0\n");
//            }
//            else{
//                if(get_next_inst(ins)->inst->Opcode==Store){
//                    int x=get_value_offset(hashMap,user_get_operand_use(&ins->inst->user,1)->Val);
//                    ins= get_next_inst(ins);
//                    printf("    vstr s0,[sp,#%d]\n",x);
//                }
//            }
//        }
//        else if(isFloatType(value1->VTy)&& isFloatType(value2->VTy)){
//            float x2=value2->pdata->var_pdata.fVal;
//            int *xx2=(int*)&x2;
//            char arr2[12]="0x";
//            sprintf(arr2+2,"%0x",*xx2);
//            printf("    ldr r2,=%s\n",arr2);
//            printf("    vcvt.s32.f32 s2,r2\n");
//
//            printf("    vmul.s32.f32 s0,s0,s2\n");
//            if(isIntType(value0->VTy)){
//                printf("    vcvt.s32.f32 r0,s0\n");
//            }
//            else{
//                if(get_next_inst(ins)->inst->Opcode==Store){
//                    int x=get_value_offset(hashMap,user_get_operand_use(&ins->inst->user,1)->Val);
//                    ins= get_next_inst(ins);
//                    printf("    vstr s0,[sp,#%d]\n",x);
//                }
//            }
//        }
//    }
//    if(!isImm(value1)&& !isImm(value2)){
//        if(isIntType(value1->VTy)&& isIntType(value2->VTy)){
//            printf("    mul r0,r0,r1\n");
//            if(isIntType(value0->VTy)){
//                ;
//            }
//            else{
//                if(get_next_inst(ins)->inst->Opcode==Store){
//                    int x=get_value_offset(hashMap,user_get_operand_use(&ins->inst->user,1)->Val);
//                    ins= get_next_inst(ins);
//                    printf("    vcvt.s32.f32 s0,r0\n");
//                    printf("    vstr s0,[sp,#%d]\n",x);
//                }
//            }
//
//        }
//        else if(isIntType(value1->VTy)&& isFloatType(value2->VTy)){
//            printf("    vcvt.s32.f32 s1,r0\n");
//            printf("    vmul.s32.f32 s0,s1,s0\n");
//            if(isIntType(value0->VTy)){
//                printf("    vcvt.s32.f32 r0,s0\n");
//            }
//            else{
//                if(get_next_inst(ins)->inst->Opcode==Store){
//                    int x=get_value_offset(hashMap,user_get_operand_use(&ins->inst->user,1)->Val);
//                    ins= get_next_inst(ins);
//                    printf("    vstr s0,[sp,#%d]\n",x);
//                }
//            }
//        }
//        else if(isFloatType(value1->VTy)&& isIntType(value2->VTy)){
//            printf("    vcvt.s32.f32 s1,r0\n");
//            printf("    vmul.s32.f32 s0,s0,s1\n");
//            if(isIntType(value0->VTy)){
//                printf("    vcvt.s32.f32 r0,s0\n");
//            }
//            else{
//                if(get_next_inst(ins)->inst->Opcode==Store){
//                    int x=get_value_offset(hashMap,user_get_operand_use(&ins->inst->user,1)->Val);
//                    ins= get_next_inst(ins);
//                    printf("    vstr s0,[sp,#%d]\n",x);
//                }
//            }
//        }
//        else if(isFloatType(value1->VTy)&& isFloatType(value2->VTy)){
//            printf("    vmul.s32.f32 s0,s0,s1\n");
//            if(isIntType(value0->VTy)){
//                printf("    vcvt.s32.f32 r0,s0\n");
//            }
//            else{
//                if(get_next_inst(ins)->inst->Opcode==Store){
//                    int x=get_value_offset(hashMap,user_get_operand_use(&ins->inst->user,1)->Val);
//                    ins= get_next_inst(ins);
//                    printf("    vstr s0,[sp,#%d]\n",x);
//                }
//            }
//        }
//    }
//
//    return  ins;
//}

InstNode * arm_trans_Div(InstNode *ins,HashMap*hashMap){
    printf("    bl __aeabi_idiv\n");
    return ins;
}

InstNode * arm_trans_Module(InstNode *ins){
    printf("arm_trans_Module\n");
    return ins;
}

InstNode * arm_trans_Call(InstNode *ins){
//    printf("CALL\n");
    printf("    bl %s\n", user_get_operand_use(&ins->inst->user,0)->Val->name);
    return ins;
}

InstNode * arm_trans_FunBegin(InstNode *ins,HashMap*hashMap){

    printf("%s:\n", user_get_operand_use(&ins->inst->user,0)->Val->name);

    if(!strcmp("main", user_get_operand_use(&ins->inst->user,0)->Val->name)){
        printf("    push {r11,lr}\n");
        printf("    mov r1,sp\n");
    }
    int x= get_siezof_sp(hashMap);
    printf("    sub sp,sp,#%d\n",4*x);
    //    在函数开始的时候要进行参数传递的str的处理
    int param_num=user_get_operand_use(&ins->inst->user,0)->Val->pdata->symtab_func_pdata.param_num;
    if(param_num!=0) {
        char name[20];
        sprintf(name, "%c", '%');
        sprintf(name + 1, "%d", param_num);
        InstNode *temp=ins;
        int ri=0;
        for (; ins != NULL && ins->inst->Opcode != Return; ins = get_next_inst(ins)) {
            Value *value0, *value1, *value2;
            switch (ins->inst->Opcode) {
                case Add:
                    value0 = &ins->inst->user.value;
                    value1 = user_get_operand_use(&ins->inst->user, 0)->Val;
                    value2 = user_get_operand_use(&ins->inst->user, 1)->Val;
                    give_param_str(hashMap,value0,name,&ri);
                    give_param_str(hashMap,value1,name,&ri);
                    give_param_str(hashMap,value2,name,&ri);
                    break;
                case Sub:
                    value0 = &ins->inst->user.value;
                    value1 = user_get_operand_use(&ins->inst->user, 0)->Val;
                    value2 = user_get_operand_use(&ins->inst->user, 1)->Val;
                    give_param_str(hashMap,value0,name,&ri);
                    give_param_str(hashMap,value1,name,&ri);
                    give_param_str(hashMap,value2,name,&ri);
                    break;
                case Mul:
                    value0 = &ins->inst->user.value;
                    value1 = user_get_operand_use(&ins->inst->user, 0)->Val;
                    value2 = user_get_operand_use(&ins->inst->user, 1)->Val;
                    give_param_str(hashMap,value0,name,&ri);
                    give_param_str(hashMap,value1,name,&ri);
                    give_param_str(hashMap,value2,name,&ri);
                    break;
                case Div:
                    value0 = &ins->inst->user.value;
                    value1 = user_get_operand_use(&ins->inst->user, 0)->Val;
                    value2 = user_get_operand_use(&ins->inst->user, 1)->Val;
                    give_param_str(hashMap,value0,name,&ri);
                    give_param_str(hashMap,value1,name,&ri);
                    give_param_str(hashMap,value2,name,&ri);
                    break;
                case Call:
                    value0 = &ins->inst->user.value;
                    value1 = user_get_operand_use(&ins->inst->user, 0)->Val;
                    give_param_str(hashMap,value0,name,&ri);
                    give_param_str(hashMap,value1,name,&ri);
                    break;
                case Store:
                    value1 = user_get_operand_use(&ins->inst->user, 0)->Val;
                    value2 = user_get_operand_use(&ins->inst->user, 1)->Val;
                    give_param_str(hashMap,value1,name,&ri);
                    give_param_str(hashMap,value2,name,&ri);
                    break;
                case Load:
                    value0 = &ins->inst->user.value;
                    value1 = user_get_operand_use(&ins->inst->user, 0)->Val;
                    give_param_str(hashMap,value0,name,&ri);
                    give_param_str(hashMap,value1,name,&ri);
                    break;
                case GIVE_PARAM:
                    value1 = user_get_operand_use(&ins->inst->user, 0)->Val;
                    give_param_str(hashMap,value1,name,&ri);
                    break;
                case LESS:
                    value0 = &ins->inst->user.value;
                    value1 = user_get_operand_use(&ins->inst->user, 0)->Val;
                    value2 = user_get_operand_use(&ins->inst->user, 1)->Val;
                    give_param_str(hashMap,value0,name,&ri);
                    give_param_str(hashMap,value1,name,&ri);
                    give_param_str(hashMap,value2,name,&ri);
                    break;
                case GREAT:
                    value0 = &ins->inst->user.value;
                    value1 = user_get_operand_use(&ins->inst->user, 0)->Val;
                    value2 = user_get_operand_use(&ins->inst->user, 1)->Val;
                    give_param_str(hashMap,value0,name,&ri);
                    give_param_str(hashMap,value1,name,&ri);
                    give_param_str(hashMap,value2,name,&ri);
                    break;
                case LESSEQ:
                    value0 = &ins->inst->user.value;
                    value1 = user_get_operand_use(&ins->inst->user, 0)->Val;
                    value2 = user_get_operand_use(&ins->inst->user, 1)->Val;
                    give_param_str(hashMap,value0,name,&ri);
                    give_param_str(hashMap,value1,name,&ri);
                    give_param_str(hashMap,value2,name,&ri);
                    break;
                case GREATEQ:
                    value0 = &ins->inst->user.value;
                    value1 = user_get_operand_use(&ins->inst->user, 0)->Val;
                    value2 = user_get_operand_use(&ins->inst->user, 1)->Val;
                    give_param_str(hashMap,value0,name,&ri);
                    give_param_str(hashMap,value1,name,&ri);
                    give_param_str(hashMap,value2,name,&ri);
                    break;
                case EQ:
                    value0 = &ins->inst->user.value;
                    value1 = user_get_operand_use(&ins->inst->user, 0)->Val;
                    value2 = user_get_operand_use(&ins->inst->user, 1)->Val;
                    give_param_str(hashMap,value0,name,&ri);
                    give_param_str(hashMap,value1,name,&ri);
                    give_param_str(hashMap,value2,name,&ri);
                    break;
                case NOTEQ:
                    value0 = &ins->inst->user.value;
                    value1 = user_get_operand_use(&ins->inst->user, 0)->Val;
                    value2 = user_get_operand_use(&ins->inst->user, 1)->Val;
                    give_param_str(hashMap,value0,name,&ri);
                    give_param_str(hashMap,value1,name,&ri);
                    give_param_str(hashMap,value2,name,&ri);
                    break;
                case XOR:
                    value0 = &ins->inst->user.value;
                    value1 = user_get_operand_use(&ins->inst->user, 0)->Val;
                    give_param_str(hashMap,value0,name,&ri);
                    give_param_str(hashMap,value1,name,&ri);
                    break;
                case zext:
                    value0 = &ins->inst->user.value;
                    value1 = user_get_operand_use(&ins->inst->user, 0)->Val;
                    give_param_str(hashMap,value0,name,&ri);
                    give_param_str(hashMap,value1,name,&ri);
                    break;
                case bitcast:
                    value0 = &ins->inst->user.value;
                    value1 = user_get_operand_use(&ins->inst->user, 0)->Val;
                    give_param_str(hashMap,value0,name,&ri);
                    give_param_str(hashMap,value1,name,&ri);
                    break;
                case GMP:
                    value0 = &ins->inst->user.value;
                    value1 = user_get_operand_use(&ins->inst->user, 0)->Val;
                    value2 = user_get_operand_use(&ins->inst->user, 1)->Val;
                    give_param_str(hashMap,value0,name,&ri);
                    give_param_str(hashMap,value1,name,&ri);
                    give_param_str(hashMap,value2,name,&ri);
                    break;
                case MEMCPY:
                    value1 = user_get_operand_use(&ins->inst->user, 0)->Val;
                    value2 = user_get_operand_use(&ins->inst->user, 1)->Val;
                    give_param_str(hashMap,value1,name,&ri);
                    give_param_str(hashMap,value2,name,&ri);
                    break;
                case GLOBAL_VAR:
                    value0 = &ins->inst->user.value;
                    value1 = user_get_operand_use(&ins->inst->user, 0)->Val;
                    value2 = user_get_operand_use(&ins->inst->user, 1)->Val;
                    give_param_str(hashMap,value0,name,&ri);
                    give_param_str(hashMap,value1,name,&ri);
                    give_param_str(hashMap,value2,name,&ri);
                    break;
                case MEMSET:
                    value1 = user_get_operand_use(&ins->inst->user, 0)->Val;
                    value2 = user_get_operand_use(&ins->inst->user, 1)->Val;
                    give_param_str(hashMap,value1,name,&ri);
                    give_param_str(hashMap,value2,name,&ri);
                    break;
                default:
                    break;
            }

        }
        ins=temp;
        for(ins= get_next_inst(ins);ins->inst->Opcode==Alloca||ins->inst->Opcode==Store;ins= get_next_inst(ins)){
            temp=ins;
        }
        ins=temp;
    }

    return ins;
}

InstNode * arm_trans_Return(InstNode *ins,InstNode *head,HashMap*hashMap){

    if(!strcmp("main", user_get_operand_use(&head->inst->user,0)->Val->name)){
        printf("    mov r11,sp\n");
        printf("    pop {r11,lr}\n");
        printf("    bx lr\n");
        return ins;
    }

    int x1= get_siezof_sp(hashMap);

    printf("    add sp,sp,#%d\n",4*x1);
    printf("    bx lr\n");
    return ins;
}

InstNode * arm_trans_Store(InstNode *ins,HashMap*hashMap){

    if(isImm(user_get_operand_use(&ins->inst->user,0)->Val)){
        if(isIntType(user_get_operand_use(&ins->inst->user,0)->Val->VTy)){
            if(imm_is_valid(user_get_operand_use(&ins->inst->user,0)->Val->pdata->var_pdata.iVal)){
                printf("    mov r0,#%d\n",user_get_operand_use(&ins->inst->user,0)->Val->pdata->var_pdata.iVal);
                int x=get_value_offset(hashMap,user_get_operand_use(&ins->inst->user,1)->Val);
                printf("    str r0,[sp,#%d]\n",x);
            } else{
                int x1=user_get_operand_use(&ins->inst->user,0)->Val->pdata->var_pdata.iVal;
                char arr[12]="0x";
                sprintf(arr+2,"%0x",x1);
                printf("    ldr r0,=%s\n",arr);
                int x=get_value_offset(hashMap,user_get_operand_use(&ins->inst->user,1)->Val);
                printf("    str r0,[sp,#%d]\n",x);
            }

        }
        else if(isFloatType(user_get_operand_use(&ins->inst->user,0)->Val->VTy)){
            float x= user_get_operand_use(&ins->inst->user,0)->Val->pdata->var_pdata.fVal;
            int *xx=(int*)&x;
            char arr[12]="0x";
            sprintf(arr+2,"%0x",*xx);
            printf("    ldr r0,=%s\n",arr);
            printf("    vcvt.s32.f32 s0,r0\n");
            int y=get_value_offset(hashMap,user_get_operand_use(&ins->inst->user,1)->Val);
            printf("    vstr s0,[sp,#%d]\n",y);
        }
    }
    else{
        if(isIntType(user_get_operand_use(&ins->inst->user,0)->Val->VTy)){
            int x=get_value_offset(hashMap,user_get_operand_use(&ins->inst->user,1)->Val);
            printf("    str r0,[sp,#%d]\n",x);
        }else{
            int x=get_value_offset(hashMap,user_get_operand_use(&ins->inst->user,1)->Val);
            printf("    vstr s0,[sp,#%d]\n",x);
        }

    }

    return ins;
}

InstNode * arm_trans_Load(InstNode *ins,HashMap*hashMap){
    if(get_next_inst(ins)->inst->Opcode==GIVE_PARAM&&regi+regs<4){
        int x= get_value_offset(hashMap,user_get_operand_use(&ins->inst->user,0)->Val);
        if(isIntType(user_get_operand_use(&ins->inst->user,0)->Val->VTy)){
            printf("    ldr r%d,[sp,#%d]\n",regi++,x);
        }
        else{
            printf("    vldr r%d,[sp,#%d]\n",regs++,x);
        }
    }
    else if(get_next_inst(ins)->inst->Opcode==GIVE_PARAM){
//        这个问题的具体处理还需要在进行思考
        int x= get_value_offset(hashMap,user_get_operand_use(&ins->inst->user,0)->Val);
        printf("    ldr r12,[sp,#%d]\n",x);
        printf("    str r12,[sp,#-%d]\n",regi);
        regi+=4;
    }
    else{
        int r=0,s=0;
        int x= get_value_offset(hashMap,user_get_operand_use(&ins->inst->user,0)->Val);
        Value *valuex= user_get_operand_use(&ins->inst->user,0)->Val;
        if(isIntType(user_get_operand_use(&ins->inst->user,0)->Val->VTy)){
            printf("    ldr r%d,[sp,#%d]\n",r++,x);
        }
        else{
            printf("    vldr s%d,[sp,#%d]\n",s++,x);
        }

        if(get_next_inst(ins)->inst->Opcode==Load){
            ins= get_next_inst(ins);
            x= get_value_offset(hashMap,user_get_operand_use(&ins->inst->user,0)->Val);
            if(isIntType(user_get_operand_use(&ins->inst->user,0)->Val->VTy)){
                printf("    ldr r%d,[sp,#%d]\n",r++,x);
            }
            else{
                printf("    vldr s%d,[sp,#%d]\n",s++,x);
            }
//            InstNode *temp= get_next_inst(ins);
//            if(temp->inst->Opcode==LESSEQ){
//                ins=temp;
//                ins= arm_trans_LESSEQ(ins,hashMap);
//            }
//            if(temp->inst->Opcode==LESS){
//                ins=temp;
//                ins= arm_trans_LESS(ins,hashMap);
//            }
//            if(temp->inst->Opcode==GREAT){
//                ins=temp;
//                ins= arm_trans_GREAT(ins,hashMap);
//            }
//            if(temp->inst->Opcode==GREATEQ){
//                ins=temp;
//                ins= arm_trans_GREATEQ(ins,hashMap);
//            }
//            if(temp->inst->Opcode==EQ){
//                ins=temp;
//                ins= arm_trans_EQ(ins,hashMap);
//            }
//            if(temp->inst->Opcode==NOTEQ){
//                ins=temp;
//                ins= arm_trans_NOTEQ(ins,hashMap);
//            }
        }
    }
    return ins;
}

InstNode * arm_trans_Alloca(InstNode *ins){
//    在汇编中，alloca不需要翻译,但是栈帧分配的时候需要用到。
    return ins;
}

InstNode * arm_trans_GIVE_PARAM(InstNode *ins){
//  这个是用来标定参数传递的，仅仅是一个指示作用
//    printf("\n");
    return ins;
}

InstNode * arm_trans_ALLBEGIN(InstNode *ins){
//    int i=ins->inst->i;
    printf("**********ALLBEGIN**************\n");
    return ins;
}

InstNode * arm_trans_LESS_GREAT_LEQ_GEQ_EQ_NEQ(InstNode *ins,HashMap*hashMap){
//a<b  a,b
    Value *value1=user_get_operand_use(&ins->inst->user,0)->Val;
    Value *value2=user_get_operand_use(&ins->inst->user,1)->Val;
    if(isImm(value1)&& isImm(value2)){
        if(isIntType(value1->VTy)&& isIntType(value2->VTy)){
            int x1=value1->pdata->var_pdata.iVal;
            int x2=value2->pdata->var_pdata.iVal;
            if(imm_is_valid(x1)&&(imm_is_valid(x2))){
                printf("    cmp #%d,#%d\n",x1,x2);
            }else if ((!imm_is_valid(x1))&&(imm_is_valid(x2))){
                char arr[12]="0x";
                sprintf(arr+2,"%0x",x1);
                printf("    ldr r1,=%s\n",arr);
                printf("    cmp r1,#%d\n",x2);
            } else if((imm_is_valid(x1))&&(!imm_is_valid(x2))){
                char arr[12]="0x";
                sprintf(arr+2,"%0x",x2);
                printf("    ldr r2,=%s\n",arr);
                printf("    cmp #%d,r2\n",x1);
            }else{
                char arr1[12]="0x";
                sprintf(arr1+2,"%0x",x1);
                char arr2[12]="0x";
                sprintf(arr2+2,"%0x",x2);
                printf("    ldr r1,=%s\n",arr1);
                printf("    ldr r2,=%s\n",arr2);
                printf("    cmp r1,r2\n");
            }
        }
        else if(isIntType(value1->VTy)&& isFloatType(value2->VTy)){
            int x1=value1->pdata->var_pdata.iVal;
            float x2=value2->pdata->var_pdata.fVal;
            if(imm_is_valid(x1)){
                printf("    mov r1,#%d\n",x1);
                printf("    vcvt.s32.f32 s1,r1\n");
                int *xx2=(int*)&x2;
                char arr2[12]="0x";
                sprintf(arr2+2,"%0x",*xx2);
                printf("    ldr r2,=%s\n",arr2);
                printf("    vcvt.s32.f32 s2,r2\n");
                printf("    vcmp s1,s2\n");
            }else{
                char arr1[12]="0x";
                sprintf(arr1+2,"%0x",x1);
                printf("    ldr r1,=%s\n",arr1);
                printf("    vcvt.s32.f32 s1,r1\n");

                int *xx2=(int*)&x2;
                char arr2[12]="0x";
                sprintf(arr2+2,"%0x",*xx2);
                printf("    ldr r2,=%s\n",arr2);
                printf("    vcvt.s32.f32 s2,r2\n");

                printf("    vcmp s1,s2\n");
            }
        }
        else if(isFloatType(value1->VTy)&& isIntType(value2->VTy)){
            float x1=value1->pdata->var_pdata.fVal;
            int x2=value2->pdata->var_pdata.iVal;
            if(imm_is_valid(x2)){
                int *xx1=(int*)&x1;
                char arr1[12]="0x";
                sprintf(arr1+2,"%0x",*xx1);
                printf("    ldr r1,=%s\n",arr1);
                printf("    vcvt.s32.f32 s1,r1\n");

                printf("    mov r2,#%d\n",x2);
                printf("    vcvt.s32.f32 s2,r2\n");

                printf("    vcmp s0,s1,s2\n");
            }else{
                int *xx1=(int*)&x1;
                char arr1[12]="0x";
                sprintf(arr1+2,"%0x",*xx1);
                printf("    ldr r1,=%s\n",arr1);
                printf("    vcvt.s32.f32 s1,r1\n");

                char arr2[12]="0x";
                sprintf(arr2+2,"%0x",x2);
                printf("    ldr r2,=%s\n",arr2);
                printf("    vcvt.s32.f32 s2,r2\n");

                printf("    vcmp s1,s2\n");
            }
        }
        else if(isFloatType(value1->VTy)&& isFloatType(value2->VTy)){
            float x1=value1->pdata->var_pdata.fVal;
            float x2=value2->pdata->var_pdata.fVal;
            int *xx1=(int*)&x1;
            char arr1[12]="0x";
            sprintf(arr1+2,"%0x",*xx1);
            printf("    ldr r1,=%s\n",arr1);
            printf("    vcvt.s32.f32 s1,r1\n");

            int *xx2=(int*)&x2;
            char arr2[12]="0x";
            sprintf(arr2+2,"%0x",*xx2);
            printf("    ldr r2,=%s\n",arr2);
            printf("    vcvt.s32.f32 s2,r2\n");

            printf("    vccmp s1,s2\n");
        }
    }
    if(isImm(value1)&& !isImm(value2)){
        if(isIntType(value1->VTy)&& isIntType(value2->VTy)){
            int x1=value1->pdata->var_pdata.iVal;
            if(imm_is_valid(x1)){
                printf("    cmp #%d,r0\n",x1);
            }else{
                char arr1[12]="0x";
                sprintf(arr1+2,"%0x",x1);
                printf("    ldr r1,=%s\n",arr1);
                printf("    cmp r1,r0\n");
            }
        }
        else if(isIntType(value1->VTy)&& isFloatType(value2->VTy)){
            int x1=value1->pdata->var_pdata.iVal;
            if(imm_is_valid(x1)){
                printf("    mov r1,#%d\n",x1);
                printf("    vcvt.s32.f32 s1,r1\n");
                printf("    vcmp s1,s0\n");
            }else{
                char arr1[12]="0x";
                sprintf(arr1+2,"%0x",x1);
                printf("    ldr r1,=%s\n",arr1);
                printf("    vcvt.s32.f32 s1,r1\n");

                printf("    vcmp s1,s0\n");
            }
        }
        else if(isFloatType(value1->VTy)&& isIntType(value2->VTy)){
            float x1=value1->pdata->var_pdata.fVal;
            int *xx1=(int*)&x1;
            char arr1[12]="0x";
            sprintf(arr1+2,"%0x",*xx1);
            printf("    ldr r1,=%s\n",arr1);
            printf("    vcvt.s32.f32 s1,r1\n");

            printf("    vcvt.s32.f32 s0,r0\n");

            printf("    vcmp s1,s0\n");
        }
        else if(isFloatType(value1->VTy)&& isFloatType(value2->VTy)){
            float x1=value1->pdata->var_pdata.fVal;
            int *xx1=(int*)&x1;
            char arr1[12]="0x";
            sprintf(arr1+2,"%0x",*xx1);
            printf("    ldr r1,=%s\n",arr1);
            printf("    vcvt.s32.f32 s1,r1\n");

            printf("    vcmp s1,s0\n");
        }

    }
    if(!isImm(value1)&& isImm(value2)){
        if(isIntType(value1->VTy)&& isIntType(value2->VTy)){
            int x2=value2->pdata->var_pdata.iVal;
            if((imm_is_valid(x2))){
                printf("    cmp r0,#%d\n",x2);
            }else{
                char arr2[12]="0x";
                sprintf(arr2+2,"%0x",x2);
                printf("    ldr r2,=%s\n",arr2);
                printf("    cmp r0,r2\n");
            }
        }
        else if(isIntType(value1->VTy)&& isFloatType(value2->VTy)){
            float x2=value2->pdata->var_pdata.fVal;
            int *xx2=(int*)&x2;
            char arr2[12]="0x";
            sprintf(arr2+2,"%0x",*xx2);
            printf("    ldr r2,=%s\n",arr2);
            printf("    vcvt.s32.f32 s2,r2\n");

            printf("    vcmp s0,s2\n");
        }
        else if(isFloatType(value1->VTy)&& isIntType(value2->VTy)){
            int x2=value2->pdata->var_pdata.iVal;
            if(imm_is_valid(x2)){
                printf("    mov r2,#%d\n",x2);
                printf("    vcvt.s32.f32 s2,r2\n");

                printf("    vcmp s0,s2\n");
            }else{
                char arr2[12]="0x";
                sprintf(arr2+2,"%0x",x2);
                printf("    ldr r2,=%s\n",arr2);
                printf("    vcvt.s32.f32 s2,r2\n");

                printf("    vcmp s0,s2\n");
            }
        }
        else if(isFloatType(value1->VTy)&& isFloatType(value2->VTy)){
            float x2=value2->pdata->var_pdata.fVal;
            int *xx2=(int*)&x2;
            char arr2[12]="0x";
            sprintf(arr2+2,"%0x",*xx2);
            printf("    ldr r2,=%s\n",arr2);
            printf("    vcvt.s32.f32 s2,r2\n");
            printf("    vcmp s0,s2\n");
        }

    }
    if(!isImm(value1)&& !isImm(value2)){
        if(isIntType(value1->VTy)&& isIntType(value2->VTy)){
            printf("    cmp r0,r1\n");
        }
        else if(isIntType(value1->VTy)&& isFloatType(value2->VTy)){
            printf("    vcvt.s32.f32 s1,r0\n");
            printf("    vcmp s1,s0\n");
        }
        else if(isFloatType(value1->VTy)&& isIntType(value2->VTy)){
            printf("    vcvt.s32.f32 s1,r0\n");
            printf("    vcmp s0,s1\n");
        }
        else if(isFloatType(value1->VTy)&& isFloatType(value2->VTy)){
            printf("    vcmp s0,s1\n");
        }
    }
    if(ins->inst->Opcode==LESS){
        InstNode *temp= get_next_inst(ins);
        if(temp->inst->Opcode==br_i1){
            ins= temp;
            int x= get_value_pdata_inspdata_false(&ins->inst->user.value);
            printf("    bge %d\n",x);
            x= get_value_pdata_inspdata_true(&ins->inst->user.value);
            printf("    b %d\n",x);
        }
    } else if(ins->inst->Opcode==GREAT){
        InstNode *temp= get_next_inst(ins);
        if(temp->inst->Opcode==br_i1){
            ins= temp;
            int x= get_value_pdata_inspdata_false(&ins->inst->user.value);
            printf("    ble %d\n",x);
            x= get_value_pdata_inspdata_true(&ins->inst->user.value);
            printf("    b %d\n",x);
        }
    } else if(ins->inst->Opcode==LESSEQ){
        InstNode *temp= get_next_inst(ins);
        if(temp->inst->Opcode==br_i1){
            ins= temp;
            int x= get_value_pdata_inspdata_false(&ins->inst->user.value);
            printf("    bgt %d\n",x);
            x= get_value_pdata_inspdata_true(&ins->inst->user.value);
            printf("    b %d\n",x);
        }
    } else if(ins->inst->Opcode==GREATEQ){
        InstNode *temp= get_next_inst(ins);
        if(temp->inst->Opcode==br_i1){
            ins= temp;
            int x= get_value_pdata_inspdata_false(&ins->inst->user.value);
            printf("    blt %d\n",x);
            x= get_value_pdata_inspdata_true(&ins->inst->user.value);
            printf("    b %d\n",x);
        }
    } else if(ins->inst->Opcode==EQ){
        InstNode *temp= get_next_inst(ins);
        if(temp->inst->Opcode==br_i1){
            ins= temp;
            int x= get_value_pdata_inspdata_false(&ins->inst->user.value);
            printf("    bne %d\n",x);
            x= get_value_pdata_inspdata_true(&ins->inst->user.value);
            printf("    b %d\n",x);
        }
    } else if(ins->inst->Opcode==NOTEQ){
        InstNode *temp= get_next_inst(ins);
        if(temp->inst->Opcode==br_i1){
            ins= temp;
            int x= get_value_pdata_inspdata_false(&ins->inst->user.value);
            printf("    beq %d\n",x);
            x= get_value_pdata_inspdata_true(&ins->inst->user.value);
            printf("    b %d\n",x);
        }
    }

    return ins;


}

//InstNode * arm_trans_GREAT(InstNode *ins,HashMap*hashMap){
//    Value *value1=user_get_operand_use(&ins->inst->user,0)->Val;
//    Value *value2=user_get_operand_use(&ins->inst->user,1)->Val;
//    if(isImm(value1)&& isImm(value2)){
//        if(isIntType(value1->VTy)&& isIntType(value2->VTy)){
//            int x1=value1->pdata->var_pdata.iVal;
//            int x2=value2->pdata->var_pdata.iVal;
//            if(imm_is_valid(x1)&&(imm_is_valid(x2))){
//                printf("    cmp #%d,#%d\n",x1,x2);
//            }else if ((!imm_is_valid(x1))&&(imm_is_valid(x2))){
//                char arr[12]="0x";
//                sprintf(arr+2,"%0x",x1);
//                printf("    ldr r1,=%s\n",arr);
//                printf("    cmp r1,#%d\n",x2);
//            } else if((imm_is_valid(x1))&&(!imm_is_valid(x2))){
//                char arr[12]="0x";
//                sprintf(arr+2,"%0x",x2);
//                printf("    ldr r2,=%s\n",arr);
//                printf("    cmp #%d,r2\n",x1);
//            }else{
//                char arr1[12]="0x";
//                sprintf(arr1+2,"%0x",x1);
//                char arr2[12]="0x";
//                sprintf(arr2+2,"%0x",x2);
//                printf("    ldr r1,=%s\n",arr1);
//                printf("    ldr r2,=%s\n",arr2);
//                printf("    cmp r1,r2\n");
//            }
//        }
//        else if(isIntType(value1->VTy)&& isFloatType(value2->VTy)){
//            int x1=value1->pdata->var_pdata.iVal;
//            float x2=value2->pdata->var_pdata.fVal;
//            if(imm_is_valid(x1)){
//                printf("    mov r1,#%d\n",x1);
//                printf("    vcvt.s32.f32 s1,r1\n");
//                int *xx2=(int*)&x2;
//                char arr2[12]="0x";
//                sprintf(arr2+2,"%0x",*xx2);
//                printf("    ldr r2,=%s\n",arr2);
//                printf("    vcvt.s32.f32 s2,r2\n");
//                printf("    vcmp s1,s2\n");
//            }else{
//                char arr1[12]="0x";
//                sprintf(arr1+2,"%0x",x1);
//                printf("    ldr r1,=%s\n",arr1);
//                printf("    vcvt.s32.f32 s1,r1\n");
//
//                int *xx2=(int*)&x2;
//                char arr2[12]="0x";
//                sprintf(arr2+2,"%0x",*xx2);
//                printf("    ldr r2,=%s\n",arr2);
//                printf("    vcvt.s32.f32 s2,r2\n");
//
//                printf("    vcmp s1,s2\n");
//            }
//        }
//        else if(isFloatType(value1->VTy)&& isIntType(value2->VTy)){
//            float x1=value1->pdata->var_pdata.fVal;
//            int x2=value2->pdata->var_pdata.iVal;
//            if(imm_is_valid(x2)){
//                int *xx1=(int*)&x1;
//                char arr1[12]="0x";
//                sprintf(arr1+2,"%0x",*xx1);
//                printf("    ldr r1,=%s\n",arr1);
//                printf("    vcvt.s32.f32 s1,r1\n");
//
//                printf("    mov r2,#%d\n",x2);
//                printf("    vcvt.s32.f32 s2,r2\n");
//
//                printf("    vcmp s0,s1,s2\n");
//            }else{
//                int *xx1=(int*)&x1;
//                char arr1[12]="0x";
//                sprintf(arr1+2,"%0x",*xx1);
//                printf("    ldr r1,=%s\n",arr1);
//                printf("    vcvt.s32.f32 s1,r1\n");
//
//                char arr2[12]="0x";
//                sprintf(arr2+2,"%0x",x2);
//                printf("    ldr r2,=%s\n",arr2);
//                printf("    vcvt.s32.f32 s2,r2\n");
//
//                printf("    vcmp s1,s2\n");
//            }
//        }
//        else if(isFloatType(value1->VTy)&& isFloatType(value2->VTy)){
//            float x1=value1->pdata->var_pdata.fVal;
//            float x2=value2->pdata->var_pdata.fVal;
//            int *xx1=(int*)&x1;
//            char arr1[12]="0x";
//            sprintf(arr1+2,"%0x",*xx1);
//            printf("    ldr r1,=%s\n",arr1);
//            printf("    vcvt.s32.f32 s1,r1\n");
//
//            int *xx2=(int*)&x2;
//            char arr2[12]="0x";
//            sprintf(arr2+2,"%0x",*xx2);
//            printf("    ldr r2,=%s\n",arr2);
//            printf("    vcvt.s32.f32 s2,r2\n");
//
//            printf("    vccmp s1,s2\n");
//        }
//    }
//    if(isImm(value1)&& !isImm(value2)){
//        if(isIntType(value1->VTy)&& isIntType(value2->VTy)){
//            int x1=value1->pdata->var_pdata.iVal;
//            if(imm_is_valid(x1)){
//                printf("    cmp #%d,r0\n",x1);
//            }else{
//                char arr1[12]="0x";
//                sprintf(arr1+2,"%0x",x1);
//                printf("    ldr r1,=%s\n",arr1);
//                printf("    cmp r1,r0\n");
//            }
//        }
//        else if(isIntType(value1->VTy)&& isFloatType(value2->VTy)){
//            int x1=value1->pdata->var_pdata.iVal;
//            if(imm_is_valid(x1)){
//                printf("    mov r1,#%d\n",x1);
//                printf("    vcvt.s32.f32 s1,r1\n");
//                printf("    vcmp s1,s0\n");
//            }else{
//                char arr1[12]="0x";
//                sprintf(arr1+2,"%0x",x1);
//                printf("    ldr r1,=%s\n",arr1);
//                printf("    vcvt.s32.f32 s1,r1\n");
//
//                printf("    vcmp s1,s0\n");
//            }
//        }
//        else if(isFloatType(value1->VTy)&& isIntType(value2->VTy)){
//            float x1=value1->pdata->var_pdata.fVal;
//            int *xx1=(int*)&x1;
//            char arr1[12]="0x";
//            sprintf(arr1+2,"%0x",*xx1);
//            printf("    ldr r1,=%s\n",arr1);
//            printf("    vcvt.s32.f32 s1,r1\n");
//
//            printf("    vcvt.s32.f32 s0,r0\n");
//
//            printf("    vcmp s1,s0\n");
//        }
//        else if(isFloatType(value1->VTy)&& isFloatType(value2->VTy)){
//            float x1=value1->pdata->var_pdata.fVal;
//            int *xx1=(int*)&x1;
//            char arr1[12]="0x";
//            sprintf(arr1+2,"%0x",*xx1);
//            printf("    ldr r1,=%s\n",arr1);
//            printf("    vcvt.s32.f32 s1,r1\n");
//
//            printf("    vcmp s1,s0\n");
//        }
//
//    }
//    if(!isImm(value1)&& isImm(value2)){
//        if(isIntType(value1->VTy)&& isIntType(value2->VTy)){
//            int x2=value2->pdata->var_pdata.iVal;
//            if((imm_is_valid(x2))){
//                printf("    cmp r0,#%d\n",x2);
//            }else{
//                char arr2[12]="0x";
//                sprintf(arr2+2,"%0x",x2);
//                printf("    ldr r2,=%s\n",arr2);
//                printf("    cmp r0,r2\n");
//            }
//        }
//        else if(isIntType(value1->VTy)&& isFloatType(value2->VTy)){
//            float x2=value2->pdata->var_pdata.fVal;
//            int *xx2=(int*)&x2;
//            char arr2[12]="0x";
//            sprintf(arr2+2,"%0x",*xx2);
//            printf("    ldr r2,=%s\n",arr2);
//            printf("    vcvt.s32.f32 s2,r2\n");
//
//            printf("    vcmp s0,s2\n");
//        }
//        else if(isFloatType(value1->VTy)&& isIntType(value2->VTy)){
//            int x2=value2->pdata->var_pdata.iVal;
//            if(imm_is_valid(x2)){
//                printf("    mov r2,#%d\n",x2);
//                printf("    vcvt.s32.f32 s2,r2\n");
//
//                printf("    vcmp s0,s2\n");
//            }else{
//                char arr2[12]="0x";
//                sprintf(arr2+2,"%0x",x2);
//                printf("    ldr r2,=%s\n",arr2);
//                printf("    vcvt.s32.f32 s2,r2\n");
//
//                printf("    vcmp s0,s2\n");
//            }
//        }
//        else if(isFloatType(value1->VTy)&& isFloatType(value2->VTy)){
//            float x2=value2->pdata->var_pdata.fVal;
//            int *xx2=(int*)&x2;
//            char arr2[12]="0x";
//            sprintf(arr2+2,"%0x",*xx2);
//            printf("    ldr r2,=%s\n",arr2);
//            printf("    vcvt.s32.f32 s2,r2\n");
//            printf("    vcmp s0,s2\n");
//        }
//
//    }
//    if(!isImm(value1)&& !isImm(value2)){
//        if(isIntType(value1->VTy)&& isIntType(value2->VTy)){
//            printf("    cmp r0,r1\n");
//        }
//        else if(isIntType(value1->VTy)&& isFloatType(value2->VTy)){
//            printf("    vcvt.s32.f32 s1,r0\n");
//            printf("    vcmp s1,s0\n");
//        }
//        else if(isFloatType(value1->VTy)&& isIntType(value2->VTy)){
//            printf("    vcvt.s32.f32 s1,r0\n");
//            printf("    vcmp s0,s1\n");
//        }
//        else if(isFloatType(value1->VTy)&& isFloatType(value2->VTy)){
//            printf("    vcmp s0,s1\n");
//        }
//    }
//    InstNode *temp= get_next_inst(ins);
//    if(temp->inst->Opcode==br_i1){
//        ins= temp;
//        int x= get_value_pdata_inspdata_false(&ins->inst->user.value);
//        printf("    ble %d\n",x);
//        x= get_value_pdata_inspdata_true(&ins->inst->user.value);
//        printf("    b %d\n",x);
//    }
//    return ins;
//
//}
//
//InstNode * arm_trans_LESSEQ(InstNode *ins,HashMap*hashMap){
//    Value *value1=user_get_operand_use(&ins->inst->user,0)->Val;
//    Value *value2=user_get_operand_use(&ins->inst->user,1)->Val;
//    if(isImm(value1)&& isImm(value2)){
//        if(isIntType(value1->VTy)&& isIntType(value2->VTy)){
//            int x1=value1->pdata->var_pdata.iVal;
//            int x2=value2->pdata->var_pdata.iVal;
//            if(imm_is_valid(x1)&&(imm_is_valid(x2))){
//                printf("    cmp #%d,#%d\n",x1,x2);
//            }else if ((!imm_is_valid(x1))&&(imm_is_valid(x2))){
//                char arr[12]="0x";
//                sprintf(arr+2,"%0x",x1);
//                printf("    ldr r1,=%s\n",arr);
//                printf("    cmp r1,#%d\n",x2);
//            } else if((imm_is_valid(x1))&&(!imm_is_valid(x2))){
//                char arr[12]="0x";
//                sprintf(arr+2,"%0x",x2);
//                printf("    ldr r2,=%s\n",arr);
//                printf("    cmp #%d,r2\n",x1);
//            }else{
//                char arr1[12]="0x";
//                sprintf(arr1+2,"%0x",x1);
//                char arr2[12]="0x";
//                sprintf(arr2+2,"%0x",x2);
//                printf("    ldr r1,=%s\n",arr1);
//                printf("    ldr r2,=%s\n",arr2);
//                printf("    cmp r1,r2\n");
//            }
//        }
//        else if(isIntType(value1->VTy)&& isFloatType(value2->VTy)){
//            int x1=value1->pdata->var_pdata.iVal;
//            float x2=value2->pdata->var_pdata.fVal;
//            if(imm_is_valid(x1)){
//                printf("    mov r1,#%d\n",x1);
//                printf("    vcvt.s32.f32 s1,r1\n");
//                int *xx2=(int*)&x2;
//                char arr2[12]="0x";
//                sprintf(arr2+2,"%0x",*xx2);
//                printf("    ldr r2,=%s\n",arr2);
//                printf("    vcvt.s32.f32 s2,r2\n");
//                printf("    vcmp s1,s2\n");
//            }else{
//                char arr1[12]="0x";
//                sprintf(arr1+2,"%0x",x1);
//                printf("    ldr r1,=%s\n",arr1);
//                printf("    vcvt.s32.f32 s1,r1\n");
//
//                int *xx2=(int*)&x2;
//                char arr2[12]="0x";
//                sprintf(arr2+2,"%0x",*xx2);
//                printf("    ldr r2,=%s\n",arr2);
//                printf("    vcvt.s32.f32 s2,r2\n");
//
//                printf("    vcmp s1,s2\n");
//            }
//        }
//        else if(isFloatType(value1->VTy)&& isIntType(value2->VTy)){
//            float x1=value1->pdata->var_pdata.fVal;
//            int x2=value2->pdata->var_pdata.iVal;
//            if(imm_is_valid(x2)){
//                int *xx1=(int*)&x1;
//                char arr1[12]="0x";
//                sprintf(arr1+2,"%0x",*xx1);
//                printf("    ldr r1,=%s\n",arr1);
//                printf("    vcvt.s32.f32 s1,r1\n");
//
//                printf("    mov r2,#%d\n",x2);
//                printf("    vcvt.s32.f32 s2,r2\n");
//
//                printf("    vcmp s0,s1,s2\n");
//            }else{
//                int *xx1=(int*)&x1;
//                char arr1[12]="0x";
//                sprintf(arr1+2,"%0x",*xx1);
//                printf("    ldr r1,=%s\n",arr1);
//                printf("    vcvt.s32.f32 s1,r1\n");
//
//                char arr2[12]="0x";
//                sprintf(arr2+2,"%0x",x2);
//                printf("    ldr r2,=%s\n",arr2);
//                printf("    vcvt.s32.f32 s2,r2\n");
//
//                printf("    vcmp s1,s2\n");
//            }
//        }
//        else if(isFloatType(value1->VTy)&& isFloatType(value2->VTy)){
//            float x1=value1->pdata->var_pdata.fVal;
//            float x2=value2->pdata->var_pdata.fVal;
//            int *xx1=(int*)&x1;
//            char arr1[12]="0x";
//            sprintf(arr1+2,"%0x",*xx1);
//            printf("    ldr r1,=%s\n",arr1);
//            printf("    vcvt.s32.f32 s1,r1\n");
//
//            int *xx2=(int*)&x2;
//            char arr2[12]="0x";
//            sprintf(arr2+2,"%0x",*xx2);
//            printf("    ldr r2,=%s\n",arr2);
//            printf("    vcvt.s32.f32 s2,r2\n");
//
//            printf("    vccmp s1,s2\n");
//        }
//    }
//    if(isImm(value1)&& !isImm(value2)){
//        if(isIntType(value1->VTy)&& isIntType(value2->VTy)){
//            int x1=value1->pdata->var_pdata.iVal;
//            if(imm_is_valid(x1)){
//                printf("    cmp #%d,r0\n",x1);
//            }else{
//                char arr1[12]="0x";
//                sprintf(arr1+2,"%0x",x1);
//                printf("    ldr r1,=%s\n",arr1);
//                printf("    cmp r1,r0\n");
//            }
//        }
//        else if(isIntType(value1->VTy)&& isFloatType(value2->VTy)){
//            int x1=value1->pdata->var_pdata.iVal;
//            if(imm_is_valid(x1)){
//                printf("    mov r1,#%d\n",x1);
//                printf("    vcvt.s32.f32 s1,r1\n");
//                printf("    vcmp s1,s0\n");
//            }else{
//                char arr1[12]="0x";
//                sprintf(arr1+2,"%0x",x1);
//                printf("    ldr r1,=%s\n",arr1);
//                printf("    vcvt.s32.f32 s1,r1\n");
//
//                printf("    vcmp s1,s0\n");
//            }
//        }
//        else if(isFloatType(value1->VTy)&& isIntType(value2->VTy)){
//            float x1=value1->pdata->var_pdata.fVal;
//            int *xx1=(int*)&x1;
//            char arr1[12]="0x";
//            sprintf(arr1+2,"%0x",*xx1);
//            printf("    ldr r1,=%s\n",arr1);
//            printf("    vcvt.s32.f32 s1,r1\n");
//
//            printf("    vcvt.s32.f32 s0,r0\n");
//
//            printf("    vcmp s1,s0\n");
//        }
//        else if(isFloatType(value1->VTy)&& isFloatType(value2->VTy)){
//            float x1=value1->pdata->var_pdata.fVal;
//            int *xx1=(int*)&x1;
//            char arr1[12]="0x";
//            sprintf(arr1+2,"%0x",*xx1);
//            printf("    ldr r1,=%s\n",arr1);
//            printf("    vcvt.s32.f32 s1,r1\n");
//
//            printf("    vcmp s1,s0\n");
//        }
//
//    }
//    if(!isImm(value1)&& isImm(value2)){
//        if(isIntType(value1->VTy)&& isIntType(value2->VTy)){
//            int x2=value2->pdata->var_pdata.iVal;
//            if((imm_is_valid(x2))){
//                printf("    cmp r0,#%d\n",x2);
//            }else{
//                char arr2[12]="0x";
//                sprintf(arr2+2,"%0x",x2);
//                printf("    ldr r2,=%s\n",arr2);
//                printf("    cmp r0,r2\n");
//            }
//        }
//        else if(isIntType(value1->VTy)&& isFloatType(value2->VTy)){
//            float x2=value2->pdata->var_pdata.fVal;
//            int *xx2=(int*)&x2;
//            char arr2[12]="0x";
//            sprintf(arr2+2,"%0x",*xx2);
//            printf("    ldr r2,=%s\n",arr2);
//            printf("    vcvt.s32.f32 s2,r2\n");
//
//            printf("    vcmp s0,s2\n");
//        }
//        else if(isFloatType(value1->VTy)&& isIntType(value2->VTy)){
//            int x2=value2->pdata->var_pdata.iVal;
//            if(imm_is_valid(x2)){
//                printf("    mov r2,#%d\n",x2);
//                printf("    vcvt.s32.f32 s2,r2\n");
//
//                printf("    vcmp s0,s2\n");
//            }else{
//                char arr2[12]="0x";
//                sprintf(arr2+2,"%0x",x2);
//                printf("    ldr r2,=%s\n",arr2);
//                printf("    vcvt.s32.f32 s2,r2\n");
//
//                printf("    vcmp s0,s2\n");
//            }
//        }
//        else if(isFloatType(value1->VTy)&& isFloatType(value2->VTy)){
//            float x2=value2->pdata->var_pdata.fVal;
//            int *xx2=(int*)&x2;
//            char arr2[12]="0x";
//            sprintf(arr2+2,"%0x",*xx2);
//            printf("    ldr r2,=%s\n",arr2);
//            printf("    vcvt.s32.f32 s2,r2\n");
//            printf("    vcmp s0,s2\n");
//        }
//
//    }
//    if(!isImm(value1)&& !isImm(value2)){
//        if(isIntType(value1->VTy)&& isIntType(value2->VTy)){
//            printf("    cmp r0,r1\n");
//        }
//        else if(isIntType(value1->VTy)&& isFloatType(value2->VTy)){
//            printf("    vcvt.s32.f32 s1,r0\n");
//            printf("    vcmp s1,s0\n");
//        }
//        else if(isFloatType(value1->VTy)&& isIntType(value2->VTy)){
//            printf("    vcvt.s32.f32 s1,r0\n");
//            printf("    vcmp s0,s1\n");
//        }
//        else if(isFloatType(value1->VTy)&& isFloatType(value2->VTy)){
//            printf("    vcmp s0,s1\n");
//        }
//    }
//    InstNode *temp= get_next_inst(ins);
//    if(temp->inst->Opcode==br_i1){
//        ins= temp;
//        int x= get_value_pdata_inspdata_false(&ins->inst->user.value);
//        printf("    bgt %d\n",x);
//        x= get_value_pdata_inspdata_true(&ins->inst->user.value);
//        printf("    b %d\n",x);
//    }
//    return ins;
//}
//
//InstNode * arm_trans_GREATEQ(InstNode *ins,HashMap*hashMap){
//    Value *value1=user_get_operand_use(&ins->inst->user,0)->Val;
//    Value *value2=user_get_operand_use(&ins->inst->user,1)->Val;
//    if(isImm(value1)&& isImm(value2)){
//        if(isIntType(value1->VTy)&& isIntType(value2->VTy)){
//            int x1=value1->pdata->var_pdata.iVal;
//            int x2=value2->pdata->var_pdata.iVal;
//            if(imm_is_valid(x1)&&(imm_is_valid(x2))){
//                printf("    cmp #%d,#%d\n",x1,x2);
//            }else if ((!imm_is_valid(x1))&&(imm_is_valid(x2))){
//                char arr[12]="0x";
//                sprintf(arr+2,"%0x",x1);
//                printf("    ldr r1,=%s\n",arr);
//                printf("    cmp r1,#%d\n",x2);
//            } else if((imm_is_valid(x1))&&(!imm_is_valid(x2))){
//                char arr[12]="0x";
//                sprintf(arr+2,"%0x",x2);
//                printf("    ldr r2,=%s\n",arr);
//                printf("    cmp #%d,r2\n",x1);
//            }else{
//                char arr1[12]="0x";
//                sprintf(arr1+2,"%0x",x1);
//                char arr2[12]="0x";
//                sprintf(arr2+2,"%0x",x2);
//                printf("    ldr r1,=%s\n",arr1);
//                printf("    ldr r2,=%s\n",arr2);
//                printf("    cmp r1,r2\n");
//            }
//        }
//        else if(isIntType(value1->VTy)&& isFloatType(value2->VTy)){
//            int x1=value1->pdata->var_pdata.iVal;
//            float x2=value2->pdata->var_pdata.fVal;
//            if(imm_is_valid(x1)){
//                printf("    mov r1,#%d\n",x1);
//                printf("    vcvt.s32.f32 s1,r1\n");
//                int *xx2=(int*)&x2;
//                char arr2[12]="0x";
//                sprintf(arr2+2,"%0x",*xx2);
//                printf("    ldr r2,=%s\n",arr2);
//                printf("    vcvt.s32.f32 s2,r2\n");
//                printf("    vcmp s1,s2\n");
//            }else{
//                char arr1[12]="0x";
//                sprintf(arr1+2,"%0x",x1);
//                printf("    ldr r1,=%s\n",arr1);
//                printf("    vcvt.s32.f32 s1,r1\n");
//
//                int *xx2=(int*)&x2;
//                char arr2[12]="0x";
//                sprintf(arr2+2,"%0x",*xx2);
//                printf("    ldr r2,=%s\n",arr2);
//                printf("    vcvt.s32.f32 s2,r2\n");
//
//                printf("    vcmp s1,s2\n");
//            }
//        }
//        else if(isFloatType(value1->VTy)&& isIntType(value2->VTy)){
//            float x1=value1->pdata->var_pdata.fVal;
//            int x2=value2->pdata->var_pdata.iVal;
//            if(imm_is_valid(x2)){
//                int *xx1=(int*)&x1;
//                char arr1[12]="0x";
//                sprintf(arr1+2,"%0x",*xx1);
//                printf("    ldr r1,=%s\n",arr1);
//                printf("    vcvt.s32.f32 s1,r1\n");
//
//                printf("    mov r2,#%d\n",x2);
//                printf("    vcvt.s32.f32 s2,r2\n");
//
//                printf("    vcmp s0,s1,s2\n");
//            }else{
//                int *xx1=(int*)&x1;
//                char arr1[12]="0x";
//                sprintf(arr1+2,"%0x",*xx1);
//                printf("    ldr r1,=%s\n",arr1);
//                printf("    vcvt.s32.f32 s1,r1\n");
//
//                char arr2[12]="0x";
//                sprintf(arr2+2,"%0x",x2);
//                printf("    ldr r2,=%s\n",arr2);
//                printf("    vcvt.s32.f32 s2,r2\n");
//
//                printf("    vcmp s1,s2\n");
//            }
//        }
//        else if(isFloatType(value1->VTy)&& isFloatType(value2->VTy)){
//            float x1=value1->pdata->var_pdata.fVal;
//            float x2=value2->pdata->var_pdata.fVal;
//            int *xx1=(int*)&x1;
//            char arr1[12]="0x";
//            sprintf(arr1+2,"%0x",*xx1);
//            printf("    ldr r1,=%s\n",arr1);
//            printf("    vcvt.s32.f32 s1,r1\n");
//
//            int *xx2=(int*)&x2;
//            char arr2[12]="0x";
//            sprintf(arr2+2,"%0x",*xx2);
//            printf("    ldr r2,=%s\n",arr2);
//            printf("    vcvt.s32.f32 s2,r2\n");
//
//            printf("    vccmp s1,s2\n");
//        }
//    }
//    if(isImm(value1)&& !isImm(value2)){
//        if(isIntType(value1->VTy)&& isIntType(value2->VTy)){
//            int x1=value1->pdata->var_pdata.iVal;
//            if(imm_is_valid(x1)){
//                printf("    cmp #%d,r0\n",x1);
//            }else{
//                char arr1[12]="0x";
//                sprintf(arr1+2,"%0x",x1);
//                printf("    ldr r1,=%s\n",arr1);
//                printf("    cmp r1,r0\n");
//            }
//        }
//        else if(isIntType(value1->VTy)&& isFloatType(value2->VTy)){
//            int x1=value1->pdata->var_pdata.iVal;
//            if(imm_is_valid(x1)){
//                printf("    mov r1,#%d\n",x1);
//                printf("    vcvt.s32.f32 s1,r1\n");
//                printf("    vcmp s1,s0\n");
//            }else{
//                char arr1[12]="0x";
//                sprintf(arr1+2,"%0x",x1);
//                printf("    ldr r1,=%s\n",arr1);
//                printf("    vcvt.s32.f32 s1,r1\n");
//
//                printf("    vcmp s1,s0\n");
//            }
//        }
//        else if(isFloatType(value1->VTy)&& isIntType(value2->VTy)){
//            float x1=value1->pdata->var_pdata.fVal;
//            int *xx1=(int*)&x1;
//            char arr1[12]="0x";
//            sprintf(arr1+2,"%0x",*xx1);
//            printf("    ldr r1,=%s\n",arr1);
//            printf("    vcvt.s32.f32 s1,r1\n");
//
//            printf("    vcvt.s32.f32 s0,r0\n");
//
//            printf("    vcmp s1,s0\n");
//        }
//        else if(isFloatType(value1->VTy)&& isFloatType(value2->VTy)){
//            float x1=value1->pdata->var_pdata.fVal;
//            int *xx1=(int*)&x1;
//            char arr1[12]="0x";
//            sprintf(arr1+2,"%0x",*xx1);
//            printf("    ldr r1,=%s\n",arr1);
//            printf("    vcvt.s32.f32 s1,r1\n");
//
//            printf("    vcmp s1,s0\n");
//        }
//
//    }
//    if(!isImm(value1)&& isImm(value2)){
//        if(isIntType(value1->VTy)&& isIntType(value2->VTy)){
//            int x2=value2->pdata->var_pdata.iVal;
//            if((imm_is_valid(x2))){
//                printf("    cmp r0,#%d\n",x2);
//            }else{
//                char arr2[12]="0x";
//                sprintf(arr2+2,"%0x",x2);
//                printf("    ldr r2,=%s\n",arr2);
//                printf("    cmp r0,r2\n");
//            }
//        }
//        else if(isIntType(value1->VTy)&& isFloatType(value2->VTy)){
//            float x2=value2->pdata->var_pdata.fVal;
//            int *xx2=(int*)&x2;
//            char arr2[12]="0x";
//            sprintf(arr2+2,"%0x",*xx2);
//            printf("    ldr r2,=%s\n",arr2);
//            printf("    vcvt.s32.f32 s2,r2\n");
//
//            printf("    vcmp s0,s2\n");
//        }
//        else if(isFloatType(value1->VTy)&& isIntType(value2->VTy)){
//            int x2=value2->pdata->var_pdata.iVal;
//            if(imm_is_valid(x2)){
//                printf("    mov r2,#%d\n",x2);
//                printf("    vcvt.s32.f32 s2,r2\n");
//
//                printf("    vcmp s0,s2\n");
//            }else{
//                char arr2[12]="0x";
//                sprintf(arr2+2,"%0x",x2);
//                printf("    ldr r2,=%s\n",arr2);
//                printf("    vcvt.s32.f32 s2,r2\n");
//
//                printf("    vcmp s0,s2\n");
//            }
//        }
//        else if(isFloatType(value1->VTy)&& isFloatType(value2->VTy)){
//            float x2=value2->pdata->var_pdata.fVal;
//            int *xx2=(int*)&x2;
//            char arr2[12]="0x";
//            sprintf(arr2+2,"%0x",*xx2);
//            printf("    ldr r2,=%s\n",arr2);
//            printf("    vcvt.s32.f32 s2,r2\n");
//            printf("    vcmp s0,s2\n");
//        }
//
//    }
//    if(!isImm(value1)&& !isImm(value2)){
//        if(isIntType(value1->VTy)&& isIntType(value2->VTy)){
//            printf("    cmp r0,r1\n");
//        }
//        else if(isIntType(value1->VTy)&& isFloatType(value2->VTy)){
//            printf("    vcvt.s32.f32 s1,r0\n");
//            printf("    vcmp s1,s0\n");
//        }
//        else if(isFloatType(value1->VTy)&& isIntType(value2->VTy)){
//            printf("    vcvt.s32.f32 s1,r0\n");
//            printf("    vcmp s0,s1\n");
//        }
//        else if(isFloatType(value1->VTy)&& isFloatType(value2->VTy)){
//            printf("    vcmp s0,s1\n");
//        }
//    }
//    InstNode *temp= get_next_inst(ins);
//    if(temp->inst->Opcode==br_i1){
//        ins= temp;
//        int x= get_value_pdata_inspdata_false(&ins->inst->user.value);
//        printf("    blt %d\n",x);
//        x= get_value_pdata_inspdata_true(&ins->inst->user.value);
//        printf("    b %d\n",x);
//    }
//    return ins;
//}
//
//InstNode * arm_trans_EQ(InstNode *ins,HashMap*hashMap){
//    Value *value1=user_get_operand_use(&ins->inst->user,0)->Val;
//    Value *value2=user_get_operand_use(&ins->inst->user,1)->Val;
//    if(isImm(value1)&& isImm(value2)){
//        if(isIntType(value1->VTy)&& isIntType(value2->VTy)){
//            int x1=value1->pdata->var_pdata.iVal;
//            int x2=value2->pdata->var_pdata.iVal;
//            if(imm_is_valid(x1)&&(imm_is_valid(x2))){
//                printf("    cmp #%d,#%d\n",x1,x2);
//            }else if ((!imm_is_valid(x1))&&(imm_is_valid(x2))){
//                char arr[12]="0x";
//                sprintf(arr+2,"%0x",x1);
//                printf("    ldr r1,=%s\n",arr);
//                printf("    cmp r1,#%d\n",x2);
//            } else if((imm_is_valid(x1))&&(!imm_is_valid(x2))){
//                char arr[12]="0x";
//                sprintf(arr+2,"%0x",x2);
//                printf("    ldr r2,=%s\n",arr);
//                printf("    cmp #%d,r2\n",x1);
//            }else{
//                char arr1[12]="0x";
//                sprintf(arr1+2,"%0x",x1);
//                char arr2[12]="0x";
//                sprintf(arr2+2,"%0x",x2);
//                printf("    ldr r1,=%s\n",arr1);
//                printf("    ldr r2,=%s\n",arr2);
//                printf("    cmp r1,r2\n");
//            }
//        }
//        else if(isIntType(value1->VTy)&& isFloatType(value2->VTy)){
//            int x1=value1->pdata->var_pdata.iVal;
//            float x2=value2->pdata->var_pdata.fVal;
//            if(imm_is_valid(x1)){
//                printf("    mov r1,#%d\n",x1);
//                printf("    vcvt.s32.f32 s1,r1\n");
//                int *xx2=(int*)&x2;
//                char arr2[12]="0x";
//                sprintf(arr2+2,"%0x",*xx2);
//                printf("    ldr r2,=%s\n",arr2);
//                printf("    vcvt.s32.f32 s2,r2\n");
//                printf("    vcmp s1,s2\n");
//            }else{
//                char arr1[12]="0x";
//                sprintf(arr1+2,"%0x",x1);
//                printf("    ldr r1,=%s\n",arr1);
//                printf("    vcvt.s32.f32 s1,r1\n");
//
//                int *xx2=(int*)&x2;
//                char arr2[12]="0x";
//                sprintf(arr2+2,"%0x",*xx2);
//                printf("    ldr r2,=%s\n",arr2);
//                printf("    vcvt.s32.f32 s2,r2\n");
//
//                printf("    vcmp s1,s2\n");
//            }
//        }
//        else if(isFloatType(value1->VTy)&& isIntType(value2->VTy)){
//            float x1=value1->pdata->var_pdata.fVal;
//            int x2=value2->pdata->var_pdata.iVal;
//            if(imm_is_valid(x2)){
//                int *xx1=(int*)&x1;
//                char arr1[12]="0x";
//                sprintf(arr1+2,"%0x",*xx1);
//                printf("    ldr r1,=%s\n",arr1);
//                printf("    vcvt.s32.f32 s1,r1\n");
//
//                printf("    mov r2,#%d\n",x2);
//                printf("    vcvt.s32.f32 s2,r2\n");
//
//                printf("    vcmp s0,s1,s2\n");
//            }else{
//                int *xx1=(int*)&x1;
//                char arr1[12]="0x";
//                sprintf(arr1+2,"%0x",*xx1);
//                printf("    ldr r1,=%s\n",arr1);
//                printf("    vcvt.s32.f32 s1,r1\n");
//
//                char arr2[12]="0x";
//                sprintf(arr2+2,"%0x",x2);
//                printf("    ldr r2,=%s\n",arr2);
//                printf("    vcvt.s32.f32 s2,r2\n");
//
//                printf("    vcmp s1,s2\n");
//            }
//        }
//        else if(isFloatType(value1->VTy)&& isFloatType(value2->VTy)){
//            float x1=value1->pdata->var_pdata.fVal;
//            float x2=value2->pdata->var_pdata.fVal;
//            int *xx1=(int*)&x1;
//            char arr1[12]="0x";
//            sprintf(arr1+2,"%0x",*xx1);
//            printf("    ldr r1,=%s\n",arr1);
//            printf("    vcvt.s32.f32 s1,r1\n");
//
//            int *xx2=(int*)&x2;
//            char arr2[12]="0x";
//            sprintf(arr2+2,"%0x",*xx2);
//            printf("    ldr r2,=%s\n",arr2);
//            printf("    vcvt.s32.f32 s2,r2\n");
//
//            printf("    vccmp s1,s2\n");
//        }
//    }
//    if(isImm(value1)&& !isImm(value2)){
//        if(isIntType(value1->VTy)&& isIntType(value2->VTy)){
//            int x1=value1->pdata->var_pdata.iVal;
//            if(imm_is_valid(x1)){
//                printf("    cmp #%d,r0\n",x1);
//            }else{
//                char arr1[12]="0x";
//                sprintf(arr1+2,"%0x",x1);
//                printf("    ldr r1,=%s\n",arr1);
//                printf("    cmp r1,r0\n");
//            }
//        }
//        else if(isIntType(value1->VTy)&& isFloatType(value2->VTy)){
//            int x1=value1->pdata->var_pdata.iVal;
//            if(imm_is_valid(x1)){
//                printf("    mov r1,#%d\n",x1);
//                printf("    vcvt.s32.f32 s1,r1\n");
//                printf("    vcmp s1,s0\n");
//            }else{
//                char arr1[12]="0x";
//                sprintf(arr1+2,"%0x",x1);
//                printf("    ldr r1,=%s\n",arr1);
//                printf("    vcvt.s32.f32 s1,r1\n");
//
//                printf("    vcmp s1,s0\n");
//            }
//        }
//        else if(isFloatType(value1->VTy)&& isIntType(value2->VTy)){
//            float x1=value1->pdata->var_pdata.fVal;
//            int *xx1=(int*)&x1;
//            char arr1[12]="0x";
//            sprintf(arr1+2,"%0x",*xx1);
//            printf("    ldr r1,=%s\n",arr1);
//            printf("    vcvt.s32.f32 s1,r1\n");
//
//            printf("    vcvt.s32.f32 s0,r0\n");
//
//            printf("    vcmp s1,s0\n");
//        }
//        else if(isFloatType(value1->VTy)&& isFloatType(value2->VTy)){
//            float x1=value1->pdata->var_pdata.fVal;
//            int *xx1=(int*)&x1;
//            char arr1[12]="0x";
//            sprintf(arr1+2,"%0x",*xx1);
//            printf("    ldr r1,=%s\n",arr1);
//            printf("    vcvt.s32.f32 s1,r1\n");
//
//            printf("    vcmp s1,s0\n");
//        }
//
//    }
//    if(!isImm(value1)&& isImm(value2)){
//        if(isIntType(value1->VTy)&& isIntType(value2->VTy)){
//            int x2=value2->pdata->var_pdata.iVal;
//            if((imm_is_valid(x2))){
//                printf("    cmp r0,#%d\n",x2);
//            }else{
//                char arr2[12]="0x";
//                sprintf(arr2+2,"%0x",x2);
//                printf("    ldr r2,=%s\n",arr2);
//                printf("    cmp r0,r2\n");
//            }
//        }
//        else if(isIntType(value1->VTy)&& isFloatType(value2->VTy)){
//            float x2=value2->pdata->var_pdata.fVal;
//            int *xx2=(int*)&x2;
//            char arr2[12]="0x";
//            sprintf(arr2+2,"%0x",*xx2);
//            printf("    ldr r2,=%s\n",arr2);
//            printf("    vcvt.s32.f32 s2,r2\n");
//
//            printf("    vcmp s0,s2\n");
//        }
//        else if(isFloatType(value1->VTy)&& isIntType(value2->VTy)){
//            int x2=value2->pdata->var_pdata.iVal;
//            if(imm_is_valid(x2)){
//                printf("    mov r2,#%d\n",x2);
//                printf("    vcvt.s32.f32 s2,r2\n");
//
//                printf("    vcmp s0,s2\n");
//            }else{
//                char arr2[12]="0x";
//                sprintf(arr2+2,"%0x",x2);
//                printf("    ldr r2,=%s\n",arr2);
//                printf("    vcvt.s32.f32 s2,r2\n");
//
//                printf("    vcmp s0,s2\n");
//            }
//        }
//        else if(isFloatType(value1->VTy)&& isFloatType(value2->VTy)){
//            float x2=value2->pdata->var_pdata.fVal;
//            int *xx2=(int*)&x2;
//            char arr2[12]="0x";
//            sprintf(arr2+2,"%0x",*xx2);
//            printf("    ldr r2,=%s\n",arr2);
//            printf("    vcvt.s32.f32 s2,r2\n");
//            printf("    vcmp s0,s2\n");
//        }
//
//    }
//    if(!isImm(value1)&& !isImm(value2)){
//        if(isIntType(value1->VTy)&& isIntType(value2->VTy)){
//            printf("    cmp r0,r1\n");
//        }
//        else if(isIntType(value1->VTy)&& isFloatType(value2->VTy)){
//            printf("    vcvt.s32.f32 s1,r0\n");
//            printf("    vcmp s1,s0\n");
//        }
//        else if(isFloatType(value1->VTy)&& isIntType(value2->VTy)){
//            printf("    vcvt.s32.f32 s1,r0\n");
//            printf("    vcmp s0,s1\n");
//        }
//        else if(isFloatType(value1->VTy)&& isFloatType(value2->VTy)){
//            printf("    vcmp s0,s1\n");
//        }
//    }
//    InstNode *temp= get_next_inst(ins);
//    if(temp->inst->Opcode==br_i1){
//        ins= temp;
//        int x= get_value_pdata_inspdata_false(&ins->inst->user.value);
//        printf("    bne %d\n",x);
//        x= get_value_pdata_inspdata_true(&ins->inst->user.value);
//        printf("    b %d\n",x);
//    }
//    return ins;
//}
//
//InstNode * arm_trans_NOTEQ(InstNode *ins,HashMap*hashMap){
//    Value *value1=user_get_operand_use(&ins->inst->user,0)->Val;
//    Value *value2=user_get_operand_use(&ins->inst->user,1)->Val;
//    if(isImm(value1)&& isImm(value2)){
//        if(isIntType(value1->VTy)&& isIntType(value2->VTy)){
//            int x1=value1->pdata->var_pdata.iVal;
//            int x2=value2->pdata->var_pdata.iVal;
//            if(imm_is_valid(x1)&&(imm_is_valid(x2))){
//                printf("    cmp #%d,#%d\n",x1,x2);
//            }else if ((!imm_is_valid(x1))&&(imm_is_valid(x2))){
//                char arr[12]="0x";
//                sprintf(arr+2,"%0x",x1);
//                printf("    ldr r1,=%s\n",arr);
//                printf("    cmp r1,#%d\n",x2);
//            } else if((imm_is_valid(x1))&&(!imm_is_valid(x2))){
//                char arr[12]="0x";
//                sprintf(arr+2,"%0x",x2);
//                printf("    ldr r2,=%s\n",arr);
//                printf("    cmp #%d,r2\n",x1);
//            }else{
//                char arr1[12]="0x";
//                sprintf(arr1+2,"%0x",x1);
//                char arr2[12]="0x";
//                sprintf(arr2+2,"%0x",x2);
//                printf("    ldr r1,=%s\n",arr1);
//                printf("    ldr r2,=%s\n",arr2);
//                printf("    cmp r1,r2\n");
//            }
//        }
//        else if(isIntType(value1->VTy)&& isFloatType(value2->VTy)){
//            int x1=value1->pdata->var_pdata.iVal;
//            float x2=value2->pdata->var_pdata.fVal;
//            if(imm_is_valid(x1)){
//                printf("    mov r1,#%d\n",x1);
//                printf("    vcvt.s32.f32 s1,r1\n");
//                int *xx2=(int*)&x2;
//                char arr2[12]="0x";
//                sprintf(arr2+2,"%0x",*xx2);
//                printf("    ldr r2,=%s\n",arr2);
//                printf("    vcvt.s32.f32 s2,r2\n");
//                printf("    vcmp s1,s2\n");
//            }else{
//                char arr1[12]="0x";
//                sprintf(arr1+2,"%0x",x1);
//                printf("    ldr r1,=%s\n",arr1);
//                printf("    vcvt.s32.f32 s1,r1\n");
//
//                int *xx2=(int*)&x2;
//                char arr2[12]="0x";
//                sprintf(arr2+2,"%0x",*xx2);
//                printf("    ldr r2,=%s\n",arr2);
//                printf("    vcvt.s32.f32 s2,r2\n");
//
//                printf("    vcmp s1,s2\n");
//            }
//        }
//        else if(isFloatType(value1->VTy)&& isIntType(value2->VTy)){
//            float x1=value1->pdata->var_pdata.fVal;
//            int x2=value2->pdata->var_pdata.iVal;
//            if(imm_is_valid(x2)){
//                int *xx1=(int*)&x1;
//                char arr1[12]="0x";
//                sprintf(arr1+2,"%0x",*xx1);
//                printf("    ldr r1,=%s\n",arr1);
//                printf("    vcvt.s32.f32 s1,r1\n");
//
//                printf("    mov r2,#%d\n",x2);
//                printf("    vcvt.s32.f32 s2,r2\n");
//
//                printf("    vcmp s0,s1,s2\n");
//            }else{
//                int *xx1=(int*)&x1;
//                char arr1[12]="0x";
//                sprintf(arr1+2,"%0x",*xx1);
//                printf("    ldr r1,=%s\n",arr1);
//                printf("    vcvt.s32.f32 s1,r1\n");
//
//                char arr2[12]="0x";
//                sprintf(arr2+2,"%0x",x2);
//                printf("    ldr r2,=%s\n",arr2);
//                printf("    vcvt.s32.f32 s2,r2\n");
//
//                printf("    vcmp s1,s2\n");
//            }
//        }
//        else if(isFloatType(value1->VTy)&& isFloatType(value2->VTy)){
//            float x1=value1->pdata->var_pdata.fVal;
//            float x2=value2->pdata->var_pdata.fVal;
//            int *xx1=(int*)&x1;
//            char arr1[12]="0x";
//            sprintf(arr1+2,"%0x",*xx1);
//            printf("    ldr r1,=%s\n",arr1);
//            printf("    vcvt.s32.f32 s1,r1\n");
//
//            int *xx2=(int*)&x2;
//            char arr2[12]="0x";
//            sprintf(arr2+2,"%0x",*xx2);
//            printf("    ldr r2,=%s\n",arr2);
//            printf("    vcvt.s32.f32 s2,r2\n");
//
//            printf("    vccmp s1,s2\n");
//        }
//    }
//    if(isImm(value1)&& !isImm(value2)){
//        if(isIntType(value1->VTy)&& isIntType(value2->VTy)){
//            int x1=value1->pdata->var_pdata.iVal;
//            if(imm_is_valid(x1)){
//                printf("    cmp #%d,r0\n",x1);
//            }else{
//                char arr1[12]="0x";
//                sprintf(arr1+2,"%0x",x1);
//                printf("    ldr r1,=%s\n",arr1);
//                printf("    cmp r1,r0\n");
//            }
//        }
//        else if(isIntType(value1->VTy)&& isFloatType(value2->VTy)){
//            int x1=value1->pdata->var_pdata.iVal;
//            if(imm_is_valid(x1)){
//                printf("    mov r1,#%d\n",x1);
//                printf("    vcvt.s32.f32 s1,r1\n");
//                printf("    vcmp s1,s0\n");
//            }else{
//                char arr1[12]="0x";
//                sprintf(arr1+2,"%0x",x1);
//                printf("    ldr r1,=%s\n",arr1);
//                printf("    vcvt.s32.f32 s1,r1\n");
//
//                printf("    vcmp s1,s0\n");
//            }
//        }
//        else if(isFloatType(value1->VTy)&& isIntType(value2->VTy)){
//            float x1=value1->pdata->var_pdata.fVal;
//            int *xx1=(int*)&x1;
//            char arr1[12]="0x";
//            sprintf(arr1+2,"%0x",*xx1);
//            printf("    ldr r1,=%s\n",arr1);
//            printf("    vcvt.s32.f32 s1,r1\n");
//
//            printf("    vcvt.s32.f32 s0,r0\n");
//
//            printf("    vcmp s1,s0\n");
//        }
//        else if(isFloatType(value1->VTy)&& isFloatType(value2->VTy)){
//            float x1=value1->pdata->var_pdata.fVal;
//            int *xx1=(int*)&x1;
//            char arr1[12]="0x";
//            sprintf(arr1+2,"%0x",*xx1);
//            printf("    ldr r1,=%s\n",arr1);
//            printf("    vcvt.s32.f32 s1,r1\n");
//
//            printf("    vcmp s1,s0\n");
//        }
//
//    }
//    if(!isImm(value1)&& isImm(value2)){
//        if(isIntType(value1->VTy)&& isIntType(value2->VTy)){
//            int x2=value2->pdata->var_pdata.iVal;
//            if((imm_is_valid(x2))){
//                printf("    cmp r0,#%d\n",x2);
//            }else{
//                char arr2[12]="0x";
//                sprintf(arr2+2,"%0x",x2);
//                printf("    ldr r2,=%s\n",arr2);
//                printf("    cmp r0,r2\n");
//            }
//        }
//        else if(isIntType(value1->VTy)&& isFloatType(value2->VTy)){
//            float x2=value2->pdata->var_pdata.fVal;
//            int *xx2=(int*)&x2;
//            char arr2[12]="0x";
//            sprintf(arr2+2,"%0x",*xx2);
//            printf("    ldr r2,=%s\n",arr2);
//            printf("    vcvt.s32.f32 s2,r2\n");
//
//            printf("    vcmp s0,s2\n");
//        }
//        else if(isFloatType(value1->VTy)&& isIntType(value2->VTy)){
//            int x2=value2->pdata->var_pdata.iVal;
//            if(imm_is_valid(x2)){
//                printf("    mov r2,#%d\n",x2);
//                printf("    vcvt.s32.f32 s2,r2\n");
//
//                printf("    vcmp s0,s2\n");
//            }else{
//                char arr2[12]="0x";
//                sprintf(arr2+2,"%0x",x2);
//                printf("    ldr r2,=%s\n",arr2);
//                printf("    vcvt.s32.f32 s2,r2\n");
//
//                printf("    vcmp s0,s2\n");
//            }
//        }
//        else if(isFloatType(value1->VTy)&& isFloatType(value2->VTy)){
//            float x2=value2->pdata->var_pdata.fVal;
//            int *xx2=(int*)&x2;
//            char arr2[12]="0x";
//            sprintf(arr2+2,"%0x",*xx2);
//            printf("    ldr r2,=%s\n",arr2);
//            printf("    vcvt.s32.f32 s2,r2\n");
//            printf("    vcmp s0,s2\n");
//        }
//
//    }
//    if(!isImm(value1)&& !isImm(value2)){
//        if(isIntType(value1->VTy)&& isIntType(value2->VTy)){
//            printf("    cmp r0,r1\n");
//        }
//        else if(isIntType(value1->VTy)&& isFloatType(value2->VTy)){
//            printf("    vcvt.s32.f32 s1,r0\n");
//            printf("    vcmp s1,s0\n");
//        }
//        else if(isFloatType(value1->VTy)&& isIntType(value2->VTy)){
//            printf("    vcvt.s32.f32 s1,r0\n");
//            printf("    vcmp s0,s1\n");
//        }
//        else if(isFloatType(value1->VTy)&& isFloatType(value2->VTy)){
//            printf("    vcmp s0,s1\n");
//        }
//    }
//    InstNode *temp= get_next_inst(ins);
//    if(temp->inst->Opcode==br_i1){
//        ins= temp;
//        int x= get_value_pdata_inspdata_false(&ins->inst->user.value);
//        printf("    beq %d\n",x);
//        x= get_value_pdata_inspdata_true(&ins->inst->user.value);
//        printf("    b %d\n",x);
//    }
//    return ins;
//}

InstNode * arm_trans_br_i1(InstNode *ins){
//    int i=ins->inst->i;
    int x= get_value_pdata_inspdata_false(&ins->inst->user.value);
    printf("    bne %d\n",x);
    x= get_value_pdata_inspdata_true(&ins->inst->user.value);
    printf("    b %d\n",x);
    return  ins;
}

InstNode * arm_trans_br(InstNode *ins){

    int x= get_value_pdata_inspdata_true(&ins->inst->user.value);
    printf("    b %d\n",x);
    return ins;
}

InstNode * arm_trans_br_i1_true(InstNode *ins){

    printf("arm_trans_br_i1_true\n");
    return ins;
}

InstNode * arm_trans_br_i1_false(InstNode *ins){

    printf("arm_trans_br_i1_false\n");
    return ins;
}

InstNode * arm_trans_Label(InstNode *ins){
//强制跳转的位置
    int x= get_value_pdata_inspdata_true(&ins->inst->user.value);
    printf("%d：\n",x);
    return ins;
}

InstNode * arm_trans_tmp(InstNode *ins){

    printf("arm_trans_tmp\n");
    return ins;
}

InstNode * arm_trans_XOR(InstNode *ins){
    printf("arm_trans_XOR\n");
    return ins;
}

InstNode * arm_trans_zext(InstNode *ins){
//i1扩展为i32
    printf("arm_trans_zext\n");
    return ins;
}

InstNode * arm_trans_bitcast(InstNode *ins){
//类型转换
    printf("arm_trans_bitcast\n");
    return ins;
}

InstNode * arm_trans_GMP(InstNode *ins,HashMap*hashMap){
//数组初始化
    if(get_next_inst((ins))->inst->Opcode==Store){
        InstNode *next= get_next_inst(ins);
        int off= user_get_operand_use(&ins->inst->user,0)->Val->pdata->var_pdata.iVal;
        int x= user_get_operand_use(&next->inst->user,0)->Val->pdata->var_pdata.iVal;
        printf("    mov r0,#%d\n",x);
        int off_sp= get_value_offset(hashMap, user_get_operand_use(&next->inst->user,1)->Val->alias)+off;
        printf("    store r0,[sp,#%d]\n",off_sp*4);
        return next;
    }
//    printf("\n");
    return ins;
}

InstNode * arm_trans_MEMCPY(InstNode *ins){

//    涉及到数组的内容
    printf("arm_trans_MEMCPY\n");
    return ins;
}

InstNode * arm_trans_zeroinitializer(InstNode *ins){

    printf("arm_trans_zeroinitializer\n");
    return ins;
}

InstNode * arm_trans_GLOBAL_VAR(InstNode *ins){

//全局变量声明
    printf("arm_trans_GLOBAL_VAR\n");
    return ins;
}

InstNode *arm_trans_Phi(InstNode *ins){

    printf("arm_trans_Phi\n");
    return ins;
}

InstNode *arm_trans_MEMSET(InstNode *ins){

    printf("arm_trans_MEMSET\n");
    return ins;
}

InstNode *_arm_translate_ins(InstNode *ins,InstNode *head,HashMap*hashMap){

    int x=ins->inst->Opcode;
    switch(x){
        case Add:
        case Sub:
        case Mul:
            return arm_trans_Add_Sub_Mul(ins,hashMap);
            break;
        case Div:
            return arm_trans_Div(ins,hashMap);
            break;
        case Module:
            return arm_trans_Module(ins);
            break;
        case Call:
            return arm_trans_Call(ins);
            break;
        case FunBegin:
            return arm_trans_FunBegin(ins,hashMap);
            break;
        case Return:
            return arm_trans_Return(ins,head,hashMap);
            break;
        case Store:
            return arm_trans_Store(ins,hashMap);
            break;
        case Load:
            return arm_trans_Load(ins,hashMap);
            break;
        case Alloca:
            return arm_trans_Alloca(ins);
            break;
        case GIVE_PARAM:
            return arm_trans_GIVE_PARAM(ins);
            break;
        case ALLBEGIN:
            return arm_trans_ALLBEGIN(ins);
            break;
        case LESS:
        case GREAT:
        case LESSEQ:
        case GREATEQ:
        case EQ:
        case NOTEQ:
            return arm_trans_LESS_GREAT_LEQ_GEQ_EQ_NEQ(ins,hashMap);
            break;
        case br_i1:
            return arm_trans_br_i1(ins);
            break;
        case br:
            return arm_trans_br(ins);
            break;
        case br_i1_true:
            return arm_trans_br_i1_true(ins);
            break;
        case br_i1_false:
            return arm_trans_br_i1_false(ins);
            break;
        case Label:
            return arm_trans_Label(ins);
            break;
        case tmp:
            return arm_trans_tmp(ins);
            break;
        case XOR:
            return arm_trans_XOR(ins);
            break;
        case zext:
            return arm_trans_zext(ins);
            break;
        case bitcast:
            return arm_trans_bitcast(ins);
            break;
        case GMP:
            return arm_trans_GMP(ins,hashMap);
            break;
        case MEMCPY:
            return arm_trans_MEMCPY(ins);
            break;
        case zeroinitializer:
            return arm_trans_zeroinitializer(ins);
            break;
        case GLOBAL_VAR:
            return arm_trans_GLOBAL_VAR(ins);
            break;
        case Phi:
            return arm_trans_Phi(ins);
            break;
        case MEMSET:
            return arm_trans_MEMSET(ins);
            break;
        default:
            break;
    }
}

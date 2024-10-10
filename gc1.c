#include <stdio.h>
#include <stdlib.h>

#define STACK_MAX 256
#define GC_THRESHOLD 10

// in this language it has two types of objects: ints and pairs
typedef enum {  // size 4
    OBJ_INT,
    OBJ_PAIR
} ObjectType; 


typedef struct sObject {    // size 20
    unsigned char marked;  // size = 4
    ObjectType type; // size = 4

    // this is to maintain a linked list which will be needed while sweeping unreachable objects
    struct sObject* next; // size = 4

    union {
        // OBJ_INT
        int value;  // size = 4

        // OBJ_PAIR
        struct {
            struct sObject* head;   // size = 4
            struct sObject* tail;   // size = 4
        };
    };

} Object;

// virtual machine with a stack space of 256
typedef struct {    // size = 1040
    Object* stack[ STACK_MAX ];  // size = 4 * 256 = 1024
    int stackSize; // size = 4

    // head of the linked list 
    Object* firstObject; // size = 4

    // total number of currently allocated objects
    int numObjects; // size = 4
    
    // number of objects required to trigger GC
    int maxObjects; // size = 4
} VM;

void assert(int condition, const char* message) {
    
    if(!condition) {
        printf("%s \n", message);
        exit(1);
    }
}

// function to create and initialize a VM
VM* newVM() {
    VM* vm = malloc(sizeof(VM));
    vm->stackSize = 0;
    vm->firstObject = NULL;
    vm->numObjects = 0;
    vm->maxObjects = GC_THRESHOLD;

    return vm;
}

// functions to manipulate stack
void push(VM* vm, Object* value) {
    assert(vm->stackSize < STACK_MAX, "Stack overflow!");
    vm->stack[vm->stackSize++] = value;
}

Object* pop(VM* vm) {
    assert(vm->stackSize > 0, "Stack underflow");
    return vm->stack[--vm->stackSize];
}

void mark(Object* object) {
    // if already marked then it will lead to a recursive cycle
    if(object->marked) 
        return;

    object->marked = 1;

    if(object->type == OBJ_PAIR) {
        mark(object->head);
        mark(object->tail);
    }
}

void markAll(VM* vm) {
    for(int i = 0; i < vm->stackSize; i++) {
        mark(vm->stack[i]);
    }
}


void sweep(VM* vm) {
    Object** object = &vm->firstObject;

    while(*object) {
        if(!(*object)->marked) {
            // this object was not reached so remove it 
            Object* unreached = *object;

            *object = unreached->next;

            free(unreached);

            vm->numObjects--;
        } else {
            // this object was reached so unmark it for next gc cycle 
            (*object)->marked = 0;
            object = &(*object)->next;
        }
    }
}

void gc(VM* vm) {
    int numObjects = vm->numObjects;

    markAll(vm);
    sweep(vm);

    vm->maxObjects = vm->numObjects == 0 ? GC_THRESHOLD : vm->numObjects * 2;

    printf("Collected objects - %d \n", numObjects - vm->numObjects); 
    printf("Remaining objects - %d \n", vm->numObjects); 
}

Object* newObject(VM* vm, ObjectType type) {

    if(vm->numObjects == vm->maxObjects)
        gc(vm);

    Object* object = malloc(sizeof(Object));
    object->type = type;
    object->marked = 0;

    // insert this object to the list of allocated objects
    object->next = vm->firstObject;
    vm->firstObject = object;
    vm->numObjects++;

    return object;
}

void pushInt(VM* vm, int intValue) {
    Object* object = newObject(vm, OBJ_INT);
    object->value = intValue;
    push(vm, object);
}

Object* pushPair(VM* vm) {
    Object* object = newObject(vm, OBJ_PAIR);
    
    // samajh nahi aaya 
    object->tail = pop(vm);
    object->head = pop(vm);

    push(vm, object);
    return object;
}

void freeVM(VM* vm) {
    vm->stackSize = 0;
    gc(vm);
    free(vm);
}

void test1() {
    printf("Test 1: Objects on stack are preserved \n");

    VM* vm = newVM();
    pushInt(vm, 1);
    pushInt(vm, 2);

    gc(vm);
    assert(vm->numObjects == 2, "Should have preserved Objects");
    freeVM(vm);
}

void test2() {
    printf("Test 2: Unreached Objects are collected \n");

    VM* vm = newVM();
    pushInt(vm, 1);
    pushInt(vm, 2);
    pop(vm);
    pop(vm);

    gc(vm);
    assert(vm->numObjects == 0, "Should have collected Objects");
    freeVM(vm);
}
int main () {

    test1();
    test2();

    return 0;
}
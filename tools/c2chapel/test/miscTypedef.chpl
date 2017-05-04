// Generated with c2chapel version 0.1.0

// Header given to c2chapel:
require "miscTypedef.h";

// Note: Generated with fake std headers

extern record simpleStruct {
  var a : c_int;
  var b : c_char;
  var c : c_void_ptr;
  var d : my_int;
  var e : my_string;
}

extern proc retStruct(a : my_int, b : my_int, r : renamedStruct) : fancyStruct;

extern proc tdPointer(ref a : fancyStruct, ref b : c_ptr(renamedStruct)) : void;

// ==== c2chapel typedefs ====

extern record fancyStruct {
  var a : c_int;
  var b : c_int;
  var c : renamedStruct;
  var d : simpleStruct;
}

extern type my_int = c_int;

extern type my_string = c_string;

extern type renamedFancy = fancyStruct;

extern type renamedStruct = simpleStruct;


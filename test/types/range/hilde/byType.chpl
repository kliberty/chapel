// byType.chpl
//
// Tests the type returned by the by operator.

writeln(typeToString((1:int.. by 10:int).type));
writeln(typeToString((1:int.. by 10:int(8)).type));
writeln(typeToString((1:int.. by 10:int(16)).type));
writeln(typeToString((1:int.. by 10:int(32)).type));
writeln(typeToString((1:int.. by 10:int(64)).type));
writeln(typeToString((1:int.. by 10:uint).type));
writeln(typeToString((1:int.. by 10:uint(8)).type));
writeln(typeToString((1:int.. by 10:uint(16)).type));
writeln(typeToString((1:int.. by 10:uint(32)).type));
writeln(typeToString((1:int.. by 10:uint(64)).type));
writeln(typeToString((1:int(8).. by 10:int).type));
writeln(typeToString((1:int(8).. by 10:int(8)).type));
writeln(typeToString((1:int(8).. by 10:int(16)).type));
writeln(typeToString((1:int(8).. by 10:int(32)).type));
writeln(typeToString((1:int(8).. by 10:int(64)).type));
writeln(typeToString((1:int(8).. by 10:uint(8)).type));
writeln(typeToString((1:int(8).. by 10:uint(16)).type));
writeln(typeToString((1:int(8).. by 10:uint(32)).type));
writeln(typeToString((1:int(16).. by 10:int).type));
writeln(typeToString((1:int(16).. by 10:int(8)).type));
writeln(typeToString((1:int(16).. by 10:int(16)).type));
writeln(typeToString((1:int(16).. by 10:int(32)).type));
writeln(typeToString((1:int(16).. by 10:int(64)).type));
writeln(typeToString((1:int(16).. by 10:uint(8)).type));
writeln(typeToString((1:int(16).. by 10:uint(16)).type));
writeln(typeToString((1:int(16).. by 10:uint(32)).type));
writeln(typeToString((1:int(32).. by 10:int).type));
writeln(typeToString((1:int(32).. by 10:int(8)).type));
writeln(typeToString((1:int(32).. by 10:int(16)).type));
writeln(typeToString((1:int(32).. by 10:int(32)).type));
writeln(typeToString((1:int(32).. by 10:int(64)).type));
writeln(typeToString((1:int(32).. by 10:uint(8)).type));
writeln(typeToString((1:int(32).. by 10:uint(16)).type));
writeln(typeToString((1:int(32).. by 10:uint(32)).type));
writeln(typeToString((1:int(64).. by 10:int).type));
writeln(typeToString((1:int(64).. by 10:int(8)).type));
writeln(typeToString((1:int(64).. by 10:int(16)).type));
writeln(typeToString((1:int(64).. by 10:int(32)).type));
writeln(typeToString((1:int(64).. by 10:int(64)).type));
writeln(typeToString((1:int(64).. by 10:uint).type));
writeln(typeToString((1:int(64).. by 10:uint(8)).type));
writeln(typeToString((1:int(64).. by 10:uint(16)).type));
writeln(typeToString((1:int(64).. by 10:uint(32)).type));
writeln(typeToString((1:int(64).. by 10:uint(64)).type));
writeln(typeToString((1:uint.. by 10:int).type));
writeln(typeToString((1:uint.. by 10:int(8)).type));
writeln(typeToString((1:uint.. by 10:int(16)).type));
writeln(typeToString((1:uint.. by 10:int(32)).type));
writeln(typeToString((1:uint.. by 10:int(64)).type));
writeln(typeToString((1:uint.. by 10:uint).type));
writeln(typeToString((1:uint.. by 10:uint(8)).type));
writeln(typeToString((1:uint.. by 10:uint(16)).type));
writeln(typeToString((1:uint.. by 10:uint(32)).type));
writeln(typeToString((1:uint.. by 10:uint(64)).type));
writeln(typeToString((1:uint(8).. by 10:int).type));
writeln(typeToString((1:uint(8).. by 10:int(8)).type));
writeln(typeToString((1:uint(8).. by 10:int(16)).type));
writeln(typeToString((1:uint(8).. by 10:int(32)).type));
writeln(typeToString((1:uint(8).. by 10:int(64)).type));
writeln(typeToString((1:uint(8).. by 10:uint(8)).type));
writeln(typeToString((1:uint(8).. by 10:uint(16)).type));
writeln(typeToString((1:uint(8).. by 10:uint(32)).type));
writeln(typeToString((1:uint(16).. by 10:int).type));
writeln(typeToString((1:uint(16).. by 10:int(8)).type));
writeln(typeToString((1:uint(16).. by 10:int(16)).type));
writeln(typeToString((1:uint(16).. by 10:int(32)).type));
writeln(typeToString((1:uint(16).. by 10:int(64)).type));
writeln(typeToString((1:uint(16).. by 10:uint(8)).type));
writeln(typeToString((1:uint(16).. by 10:uint(16)).type));
writeln(typeToString((1:uint(16).. by 10:uint(32)).type));
writeln(typeToString((1:uint(32).. by 10:int).type));
writeln(typeToString((1:uint(32).. by 10:int(8)).type));
writeln(typeToString((1:uint(32).. by 10:int(16)).type));
writeln(typeToString((1:uint(32).. by 10:int(32)).type));
writeln(typeToString((1:uint(32).. by 10:int(64)).type));
writeln(typeToString((1:uint(32).. by 10:uint(8)).type));
writeln(typeToString((1:uint(32).. by 10:uint(16)).type));
writeln(typeToString((1:uint(32).. by 10:uint(32)).type));
writeln(typeToString((1:uint(64).. by 10:int).type));
writeln(typeToString((1:uint(64).. by 10:int(8)).type));
writeln(typeToString((1:uint(64).. by 10:int(16)).type));
writeln(typeToString((1:uint(64).. by 10:int(32)).type));
writeln(typeToString((1:uint(64).. by 10:int(64)).type));
writeln(typeToString((1:uint(64).. by 10:uint).type));
writeln(typeToString((1:uint(64).. by 10:uint(8)).type));
writeln(typeToString((1:uint(64).. by 10:uint(16)).type));
writeln(typeToString((1:uint(64).. by 10:uint(32)).type));
writeln(typeToString((1:uint(64).. by 10:uint(64)).type));

from xlang_algorithm import algorithm

g =	 algorithm.graph()
g.AddEdge(0, 1, 2);
g.AddEdge(1, 2, 3);
g.AddEdge(2, 3, 5);
g.AddEdge(3, 4, 2);
g.AddEdge(4, 0, 4);
p = g.Prim_MST(0)
print(p)
c = g.PTreeChildren(p,1)
print(c)
sub = g.SubTree(p,1)
sub_tree = sub[0]
sub_map = sub[1]
print("end")


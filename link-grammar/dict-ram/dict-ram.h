

Dict_node * dict_node_new(void);
Dict_node * strict_lookup_list(const Dictionary dict, const char *s);
Dict_node * dsw_tree_to_vine (Dict_node *root);
Dict_node * dsw_vine_to_tree (Dict_node *root, int size);


Exp * make_zeroary_node(Pool_desc *mp);
Exp * make_op_Exp(Pool_desc *mp, Exp_type t);
Exp * make_and_node(Pool_desc *mp, Exp* nl, Exp* nr);
Exp * make_or_node(Pool_desc *mp, Exp* nl, Exp* nr);
Exp * make_optional_node(Pool_desc *mp, Exp *e);

void add_define(Dictionary dict, const char *name, const char *value);

void add_category(Dictionary dict, Exp *e, Dict_node *dn, int n);



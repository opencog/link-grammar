  void generate_power_pruning_right(int w, Exp* exp, int& dfs_position, std::vector<int> empty_conditions, char* name, char* var);
  void generate_power_pruning_left(int w, Exp* exp, int& dfs_position, std::vector<int> empty_conditions, char* name, char* var, int w_r, int pos_r, char* name_r);

void SATEncoder::generate_power_pruning_right(int w, Exp* exp, int& dfs_position, std::vector<int> empty_conditions, char* name, char* var) {
  if (exp->type == CONNECTOR_type) {
    dfs_position++;
    if (exp->dir == '+' && !exp->multi) {
      for (int w_l = w + 2; w_l < _sent->length; w_l++) {
	if (_sent->word[w_l].x == NULL)
	  continue;

	char name_l[MAX_VARIABLE_NAME];
	sprintf(name_l, "w%d", w_l);
	bool join = _sent->word[w_l].x->next != NULL;
	Exp* e_l = join ? join_alternatives(w_l) : _sent->word[w_l].x->exp;
	int dfs_position_l = 0;
	generate_power_pruning_left(w_l, e_l, dfs_position_l, empty_conditions, name_l, NULL,
				    w, dfs_position, exp->u.string);
	if (join)
	  free_alternatives(e_l);
      }
    }
  } else if (exp->type == OR_type) {
    if (exp->u.l != NULL && exp->u.l->next == NULL) {
      /* unary or - skip */
      generate_power_pruning_right(w, exp->u.l->e,dfs_position, empty_conditions, name, var);
    } else {
      int i;
      E_list* l;
      for (l = exp->u.l, i = 0; l != NULL; l = l->next, i++) {
	char new_var[MAX_VARIABLE_NAME];
	if (var != NULL) {
	  sprintf(new_var, "%s_d%d", var, i);
	} else {
	  sprintf(new_var, "d%d", i);
	}

	generate_power_pruning_right(w, l->e, dfs_position, empty_conditions, name, new_var);
      }
    }
  } else if (exp->type == AND_type) {
    if (exp->u.l != NULL && exp->u.l->next == NULL) {
      /* unary and - skip */
      generate_power_pruning_right(w, exp->u.l->e,dfs_position, empty_conditions, name, var);
    } else {
      int i;
      E_list* l;
      for (l = exp->u.l, i = 0; l != NULL; l = l->next, i++) {
	char new_var[MAX_VARIABLE_NAME];
	if (var != NULL) {
	  sprintf(new_var, "%s_c%d", var, i);
	} else {
	  sprintf(new_var, "c%d", i);
	}
	generate_power_pruning_right(w, l->e, dfs_position, empty_conditions, name, new_var);
	empty_conditions.push_back(-_variables->epsilon(name, new_var, '+'));
      }
    }
  }
}

void SATEncoder::generate_power_pruning_left(int w, Exp* exp, int& dfs_position, std::vector<int> empty_conditions, char* name, char* var, int w_r, int pos_r, char* name_r) {
  //  cout << "TDL: ";
  //  print_expression(exp);
  //  cout << endl;

  if (exp->type == CONNECTOR_type) {
    dfs_position++;
    if (exp->dir == '-' && !exp->multi) {
      Connector c1, c2;

      init_connector(&c1);
      c1.label = NORMAL_LABEL;
      c1.priority = THIN_priority;
      c1.string = name_r;

      init_connector(&c2);
      c2.label = NORMAL_LABEL;
      c2.priority = THIN_priority;
      c2.string = exp->u.string;

      //     cout << name_r << "_" << w_r << "_" << pos_r << " vs " 
      //	   << exp->u.string << "_" << w << "_" << dfs_position << endl;

      if (!WordTag::match(c1, '+', c2))
	return;
	

      cout << "POS: ";
      cout << name_r << "_." << w_r << "._." << pos_r << ". vs " 
	   << exp->u.string << "_." << w << "._." << dfs_position << endl;

      empty_conditions.push_back(-_variables->link(w_r, pos_r, name_r, w, 
						   dfs_position, exp->u.string));
      generate_or(empty_conditions);
    }
  } else if (exp->type == OR_type) {
    if (exp->u.l != NULL && exp->u.l->next == NULL) {
      /* unary or - skip */
      generate_power_pruning_left(w, exp->u.l->e,dfs_position, empty_conditions, name, var, w_r, pos_r, name_r);
    } else {
      int i;
      E_list* l;
      for (l = exp->u.l, i = 0; l != NULL; l = l->next, i++) {
	char new_var[MAX_VARIABLE_NAME];
	if (var != NULL) {
	  sprintf(new_var, "%s_d%d", var, i);
	} else {
	  sprintf(new_var, "d%d", i);
	}

	generate_power_pruning_left(w, l->e, dfs_position, empty_conditions, name, new_var, w_r, pos_r, name_r);
      }
    }
  } else if (exp->type == AND_type) {
    if (exp->u.l != NULL && exp->u.l->next == NULL) {
      /* unary and - skip */
      generate_power_pruning_left(w, exp->u.l->e,dfs_position, empty_conditions, name, var, w_r, pos_r, name_r);
    } else {
      int i;
      E_list* l;
      for (l = exp->u.l, i = 0; l != NULL; l = l->next, i++) {
	char new_var[MAX_VARIABLE_NAME];
	if (var != NULL) {
	  sprintf(new_var, "%s_c%d", var, i);
	} else {
	  sprintf(new_var, "c%d", i);
	}
	generate_power_pruning_left(w, l->e, dfs_position, empty_conditions, name, new_var, w_r, pos_r, name_r);
	empty_conditions.push_back(-_variables->epsilon(name, new_var, '-'));
      }
    }
  }
}


bool SATEncoder::pp_match(const char* name) {
  //  cout << "PPMATCH: " << name << endl;
  for (int w = 0; w < _sent->length; w++) 
    if (_word_tags[w].pp_check(name))
      return true;
  return false;
}

bool SATEncoder::pp_check(const char* connector) {
  if (_sent->dict->postprocessor == NULL) 
    return true;

  pp_knowledge * knowledge;
  knowledge = _sent->dict->postprocessor->knowledge;

  for (int i=0; i<knowledge->n_contains_one_rules; i++) {
    pp_rule rule;
    char * selector;
    pp_linkset * link_set;

    rule = knowledge->contains_one_rules[i]; /* the ith rule */
    selector = rule.selector;				/* selector string for this rule */
    link_set = rule.link_set;				/* the set of criterion links */
    if (strchr(selector, '*') != NULL) 
      continue;  /* If it has a * forget it */
    if (!post_process_match(selector, connector)) continue;
    cout << "Trigger: " << connector << endl;
    pp_linkset_node *p;

    bool match_possible = false;
    for (int hashval = 0; hashval < link_set->hash_table_size && !match_possible; hashval++) {
      for (p = link_set->hash_table[hashval]; p!=NULL && !match_possible; p=p->next) {
	const char * t;
	char name[20], *s;
	strncpy(name, p->str, sizeof(name)-1);
	name[sizeof(name)-1] = '\0';

	cout << "NAME: " << name << endl;

	for (s = name; isupper((int)*s); s++)
	  ;
	for (;*s != '\0'; s++) 
	  if (*s != '*') 
	    *s = '#';
	for (s = name, t = p->str; isupper((int) *s); s++, t++)
	  ;
	
	int bad = 0;
	int n_subscripts = 0;
	for (;*s != '\0' && bad==0; s++, t++) {
	  if (*s == '*')
	    continue;
	  n_subscripts++;
	  /* after the upper case part, and is not a * so must be a regular subscript */
	  *s = *t;
	  if (!pp_match(name))
	    bad++;
	  *s = '#';
	}

	if (n_subscripts == 0) {
	  /* now we handle the special case which occurs if there
	     were 0 subscripts */
	  if (!pp_match(name)) 
	    bad++;
	}

	/* now if bad==0 this criterion link does the job
	   to satisfy the needs of the trigger link */
	if (bad == 0) 
	  match_possible = true;
      }
    }
    if (!match_possible) {
      cout << "THIS ONE IS FALSE\n" << endl;
      return false;
    }
  }
  return true;
}

bool pp_match(const char* name);
bool pp_check(const char* connector);
bool pp_check(const char* connector);

bool WordTag::pp_check(const char* connector) {
  std::vector<PositionConnector>::const_iterator i;
  for (i = _left_connectors.begin(); i != _left_connectors.end(); i++)
    if (post_process_match(connector, i->connector->string))
      return true;
  for (i = _right_connectors.begin(); i != _right_connectors.end(); i++)
    if (post_process_match(connector, i->connector->string))
      return true;	
  return false;
}


static void split(const std::string& str, char c, std::vector<std::string>& parts) {
  const char *s, *e;
  e = str.c_str();
  while(1) {
    s = e + 1; 
    e = strchr(s, c);
    if (!e) {
      parts.push_back(str.substr(s - str.c_str()));
      break;
    } else
      parts.push_back(str.substr(s - str.c_str(), e - s));
  }
}


static char* link_label(const std::string& link_var) {
    std::vector<std::string> parts;
    split(link_var, '_', parts);
    return construct_link_label(parts[3].c_str(), parts[6].c_str());
}

static int link_left_word(const std::string& link_var) {
    std::vector<std::string> parts;
    split(link_var, '_', parts);
    return atoi(parts[1].c_str());
}

static int link_right_word(const std::string& link_var) {
    std::vector<std::string> parts;
    split(link_var, '_', parts);
    return atoi(parts[4].c_str());
}


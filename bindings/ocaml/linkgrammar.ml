(****************************************************************)
(* Module [Linkgrammar]: Provides an Ocaml interface to         *)
(* LinkGrammar - A parser for english sentences                 *)
(*                                                              *)
(* For more info on LinkGrammar                                 *)
(* refer: http://www.link.cs.cmu.edu/link/                      *)
(*                                                              *)
(* Author: Ramu Ramamurthy ramu_ramamurthy at yahoo dot com     *)
(* (C) 2006                                                     *)
(*                                                              *)
(* This is released under the BSD license                       *)
(****************************************************************)


(**
   {b ------parse options and operations on it------}
*)

type parseOptions
;;

external poCreateBase: unit -> parseOptions = "po_create";;
external freePo : parseOptions -> unit = "free_po";;

let poCreate () =
  let v = poCreateBase () in
  let () = Gc.finalise freePo v in
    v
;;

external poGetVerbosity : parseOptions -> int = "po_get_verbosity";;
external poSetVerbosity : parseOptions -> int -> unit = "po_set_verbosity";;
external poGetLinkageLimit : parseOptions -> int = "po_get_linkage_limit";;
external poSetLinkageLimit :  parseOptions -> int -> unit = "po_set_linkage_limit";;
external poGetDisjunctCost : parseOptions -> int = "po_get_disjunct_cost";;
external poSetDisjunctCost : parseOptions -> int -> unit = "po_set_disjunct_cost";;
external poGetMinNullCount : parseOptions -> int = "po_get_min_null_count";;
external poSetMinNullCount : parseOptions -> int -> unit = "po_set_min_null_count";;
external poGetMaxNullCount : parseOptions -> int = "po_get_max_null_count";;
external poSetMaxNullCount : parseOptions -> int -> unit = "po_set_max_null_count";;
external poGetNullBlock : parseOptions -> int = "po_get_null_block";;
external poSetNullBlock : parseOptions -> int -> unit = "po_set_null_block";;
external poGetShortLength : parseOptions -> int = "po_get_short_length";;
external poSetShortLength : parseOptions -> int -> unit = "po_set_short_length";;
external poGetIslandsOk : parseOptions -> int = "po_get_islands_ok";;
external poSetIslandsOk : parseOptions -> int -> unit = "po_set_islands_ok";;
external poGetMaxParseTime : parseOptions -> int = "po_get_max_parse_time";;
external poSetMaxParseTime : parseOptions -> int -> unit = "po_set_max_parse_time";;
external poTimerExpired : parseOptions -> int = "po_get_timer_expired";;
external poResetResources : parseOptions -> unit = "po_reset_resources";;
external poGetAllowNull : parseOptions -> int = "po_get_allow_null";;
external poSetAllowNull : parseOptions -> int -> unit = "po_set_allow_null";;
external poGetAllShortConnectors : parseOptions -> int = "po_get_all_short_connectors";;
external poSetAllShortConnectors : parseOptions -> int -> unit = "po_set_all_short_connectors";;

(**
   {b -------dictionary and operations on it--------}
*)
type dictionary;;

external dictCreateBase : string -> string -> string -> string -> dictionary = "dict_create";;
external freeDict : dictionary -> unit = "free_dict";;

let dictCreate s1 s2 s3 s4 =
  let v = dictCreateBase s1 s2 s3 s4 in
  let () = Gc.finalise freeDict v in
    v
;;

(**
   {b -------sentences and operations on it--------}
*)
type sentence;;

external sentCreateBase : string -> dictionary -> sentence = "sent_create";;
external freeSent : sentence -> unit = "free_sentence";;

let sentCreate dict s =
  let v = sentCreateBase s dict in
  let () = Gc.finalise freeSent v in
    v
;;

external sentParse : sentence -> parseOptions -> int = "sent_parse";;
external sentLength : sentence -> int = "sent_length";;
external sentGetWord : sentence -> int -> string = "sent_get_word";;
external sentNullCount : sentence -> int = "sent_null_count";;
external sentNumLinkagesFound : sentence -> int = "sent_num_linkages_found";;
external sentNumValidLinkages : sentence -> int = "sent_num_valid_linkages";;
external sentNumLinkagesPP : sentence -> int = "sent_num_linkages_post_processed";;
external sentNumViolations : sentence -> int -> int = "sent_num_violations";;
external sentDisjunctCost : sentence -> int -> int = "sent_disjunct_cost";;

let sentGetWords sent = 
  let wList = ref [] in
  let () = 
    for i = 0 to (sentLength sent)-1 do
      wList := List.append !wList [sentGetWord sent i]
    done
  in
    !wList

(**
   {b -------linkage and operations on it--------}
*)
type linkage

external linkageCreateBase : sentence -> int -> parseOptions -> linkage = "link_create";;
external freeLinkage : linkage -> unit = "free_linkage";;

let linkageCreate sent ith po =
  let v = linkageCreateBase sent ith po in
  let () = Gc.finalise freeLinkage v in
    v
;;


external linkageGetNumSublinkages : linkage -> int = "link_get_num_sublinkages";;
external linkageSetSublinkage : linkage -> int -> unit = "link_set_current_sublinkage";;
external linkageComputeUnion : linkage -> unit = "link_compute_union";;

external linkageGetNumWords : linkage -> int = "link_get_num_words";;
external linkageGetWord : linkage -> int -> string = "link_get_word";;
let linkageGetWords link =
  let wList = ref [] in
  let () =
    for i = 0 to ((linkageGetNumWords link) - 1) do
      wList := List.append !wList [linkageGetWord link i]
    done
  in
    !wList
;;

external linkageGetNumLinks : linkage -> int = "link_get_num_links";;
external linkageGetLinkLength : linkage -> int -> int = "link_get_link_length";;
external linkageGetLinkLword : linkage -> int -> int = "link_get_link_lword";;
external linkageGetLinkRword : linkage -> int -> int = "link_get_link_rword";;
external linkageGetLinkLabel : linkage -> int -> string = "link_get_link_label";;
external linkageGetLinkLlabel : linkage -> int -> string = "link_get_link_llabel";;
external linkageGetLinkRlabel : linkage -> int -> string = "link_get_link_rlabel";;

external linkagePrintDiagram : linkage -> string = "link_print_diagram";;
external linkagePrintPostscript : linkage -> int -> string = "link_print_postscript";;
external linkagePrintLinksAndDomains : linkage -> string = "link_print_links_and_domains";;


external linkageGetNumDomains : linkage -> int -> int = "link_get_link_num_domains"
external linkageGetLinkDomainNameI : linkage -> int -> int -> string = "link_get_link_domain_name_i"
external linkageGetViolationName : linkage -> string = "link_get_violation_name"

let linkageGetLinkDomainNames linkage link_ind =
  let wList = ref [] in
  let () =
    for i = 0 to ((linkageGetNumDomains linkage link_ind) - 1) do
      wList := List.append !wList [linkageGetLinkDomainNameI linkage link_ind i]
    done
  in
    !wList
;;
  

external linkageUnusedWordCost : linkage -> int = "link_unused_word_cost"
external linkageDisjunctCost : linkage -> int = "link_disjunct_cost"
external linkageAndCost : linkage -> int = "link_and_cost"
external linkageLinkCost : linkage -> int = "link_link_cost"

(**
   {b -------constituent tree and operations on it--------}
*)
type cnode

external getCnodeRootBase : linkage -> cnode = "get_constituent_tree";;
external freeCnode : cnode -> unit = "free_cnode_tree";;

let getCnodeRoot link =
  let v = getCnodeRootBase link in
  let () = Gc.finalise freeCnode v in
    v
;;

external getCnodeLabel : cnode -> string = "get_cnode_label";;
external getCnodeStart : cnode -> int = "get_cnode_start";;
external getCnodeEnd : cnode -> int = "get_cnode_end";;
external isCnodeLeaf : cnode -> bool = "is_leaf_cnode";;
external getCnodeNumChildren : cnode -> int = "get_cnode_num_children";;
external getCnodeChild : cnode -> int -> cnode = "get_cnode_childi";;


type cTree = 
  | Node of string * int * int * cTree list
;;

let getConstituentTree link = 
  let rec buildTree cn =
    if (isCnodeLeaf cn) then
      Node ((getCnodeLabel cn),(getCnodeStart cn),(getCnodeEnd cn), [])
    else
      let childList = ref [] in
      let () =
	for i = 0 to ((getCnodeNumChildren cn)-1) do
	  childList := (List.append !childList [buildTree (getCnodeChild cn i)])
	done in
	Node ((getCnodeLabel cn),(getCnodeStart cn),(getCnodeEnd cn), !childList)
  in
  let root = getCnodeRoot link in
  let tree = buildTree root in
    tree
;;

let printIndent indent = 
  let () = 
    for i = 1 to indent do
      Printf.printf "  "
    done
  in
    ()
;;
  
let rec printTree indent tree = 
  let Node(str, st, en, lis) = tree in
    if (List.length lis) = 0 then
      let () = printIndent indent in
      let () = Printf.printf "%s\n" str;flush stdout in
	()
    else
      let () = printIndent indent in
      let () = Printf.printf "%s\n" str;flush stdout in
	List.iter (printTree (indent+1)) lis
;;

let printConstituentTree link = 
  let tree = getConstituentTree link in
    printTree 1 tree
;;


    
    






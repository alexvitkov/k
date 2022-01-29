const Value = 0;
const Tail = 8;
const Sizeof_Cons = 16;

fn Append(list, value) {
  var new_node;
  var tail;

  if get(list) {
    set tail = (get(list)) + Tail;
    Append(tail, value);
  }
  else {
    set new_node = malloc(Sizeof_Cons);
    set new_node->Value = value;
    set new_node->Tail = 0;
    set list->NULL = new_node;
  }

  return get(list);
}

fn Length(list) {
  if list == 0 {
    return 0;
  } else {
    return 1 + (Length(list->Tail));
  }
}

fn Nth(list, n) {
  if n == 0 {
    return list->Value;
  } else {
    return Nth(list->Tail, n - 1);
  }
}
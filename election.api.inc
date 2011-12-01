<?php

function _elec_get_format_from_accept_header($accept){
  if(strpos($accept, 'application/json') !==FALSE)
    return 'json';
  elseif(strpos($accept, 'text/html') !==FALSE)
    return 'html';
  elseif(strpos($accept, 'text/csv') !==FALSE)
    return 'csv';
  else{
    header('HTTP/1.0 406 Not Accepted');
    print "The server does not know how to respond to this request\n";
    print_r($_SERVER);
    exit;
  }
}

function _elec_return_as($data, $format, $type){
  switch($format){
    case 'json': 
      print str_replace("\/", "/", json_encode($data));
      exit;
    case 'html':
      return _elec_as_html($data, $type);
      break;
    case 'csv':
      return _elec_as_csv($data);
      break;
    default:
      return _elec_404();
  }
}

function _elec_arg_parse(){
  $cache = _elec_check_cache();
  $accept = $_SERVER['HTTP_ACCEPT'];
  $format = _elec_get_format_from_accept_header($accept);
  $type = 'season';
  if ($cache) {
    $data = unserialize($cache->data);
    _elec_send_modify_expire_headers($cache->expires, $cache->last_update);
    _elec_return_as($data, $format, $type);
    return;
  }
  $args = func_get_args();
  $season = array_shift($args);
  if ( empty($args) ){
    _elec_send_modify_expire_headers();
    return  _elec_return_as(_elec_get_season($season), $format, $type);
  }
  else{
    dpm($args);
  }
  return 'main parse';
}

function _elec_as_html($data,$type){
  switch($type){
    case 'season':
      _elec_html_season($data);
  }
  exit;
}

function _elec_html_season($data){
  $data = (object)$data;
  $election_events="<ul>\n";
  foreach($data->election_events as $election_event){
    $election_event = (object)$election_event;
    $election_events .= "\t<li>\n";
    $election_events .= "\t\t<a href=\"" . $election_event->link . "\" class=\"election-event\">$election_event->title</a>\n";
    $election_events .= "\t</li>\n";
  }
  $election_events .= "</ul>";
  print <<<SEASON
<!DOCTYPE html>
<head>
  <title>$data->title</title>
  <link href="$data->link" type="canonical" />
</head>
<body class="election-season">
  <h1 class="title">$data->title</h1>
  <h2>Election Events</h2>
  $election_events
</body>
SEASON;
  exit;
}

function _elec_as_csv($data){
  print_r($data);
  exit;
}

function _elec_get_season($season_slug){
  $season_node = node_load(array('title'=> _elec_slug_to_title($season_slug), 'type'=>'elec_election_season'), NULL, TRUE); 
  if (!$season_node){
    _elec_404("Season not found");
  }

  $season_link = $_GET['q'];

  $election_events = array();
  foreach($season_node->field_elec_election_events as $key => $election_event){
    $e_node = node_load(array('nid'=>$election_event['nid']));
    $e_event = array(
      'title' => $e_node->title,
      'link'  => $season_link . '/' . _elec_title_to_slug($e_node->title),
    );
    $election_events[] = $e_event;
  }
  $data = array(
    'title' => $season_node->title, 
    'link' => $season_link,
    'election_events' => $election_events,
  );
  _elec_write_cache($_GET['q'], $data);
  return $data;
}


function _elec_404($message){
  header('HTTP/1.0 404 Not Found');
  print $message . "\n";
  exit;
}

function _elec_slug_to_title($slug){
  return str_replace('-',' ', $slug);
}


function _elec_title_to_slug($title){
  return strtolower(str_replace(' ', '-', $title));
}


function _elec_write_cache($uri, $data){
  $query = 'INSERT INTO {election_cache} (uri,last_update,expires,data) VALUES ("%s", %d, %d, "%s") ON DUPLICATE KEY UPDATE last_update=%d, expires=%d, data="%s"';
  $last_update = time();
  $expires = $last_update + variable_get('elec_expire_time', 60);
  db_query($query, $uri, $last_update, $expires, serialize($data), $last_update, $expires, serialize($data));
}

function _elec_check_cache($uri = NULL){
  if( ! $uri )
    $uri = $_GET['q'];
  return db_fetch_object(db_query('SELECT data, last_update,expires from {election_cache} where uri="%s" AND expires > %d', $uri, time()));
}

function _elec_send_modify_expire_headers($expires = null, $last_modified = null){
  $last_modified = ($last_modified) ? $last_modified : time();
  $expires = ($expires) ? $expires : time() + variable_get('elec_expire_time', 60);
  header('Expires: ' . date("D, d M Y H:i:s e", $expires));
  header('Last-Modified: ' . date("D, d M Y H:i:s e", $last_modified));
}
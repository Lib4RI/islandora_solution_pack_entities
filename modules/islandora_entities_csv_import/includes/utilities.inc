<?php

/**
 * @file
 * Creates Islandora Entity Objects from CSV.
 */

/**
 * Created Entities from supplied csv file.
 *
 * @param url $path
 *   Path to file
 * @param string $collection_pid
 *   PID of collection where created objects are to be stored.
 */
function islandora_entities_import_csv_batch($path, $collection_pid) {
  $scholars = islandora_entities_read_csv($path);
  $batch = array(
    'title' => t("Creating Scholar Objects"),
    'progress_message' => t('Adding @current scholars out of @total.'),
    'operations' => array(),
    'file' => drupal_get_path('module', 'islandora_entities_csv_import') . '/includes/utilities.inc',
  );
  foreach ($scholars as $scholar) {
    $batch['operations'][] = array(
      'islandora_entities_build_scholar_object',
      array($scholar, $collection_pid),
    );
  }
  batch_set($batch);
}

/**
 * Creates associative array representing scholar data.
 *
 * @param url $path
 *   Path to CSV file
 *
 * @return array
 *   array of associative arrays containing header => value pairs
 */
function islandora_entities_read_csv($path) {
  ini_set("auto_detect_line_endings", "1");
  $scholars = array();
  if (($handle = fopen($path, "r")) !== FALSE) {
	/* [DP]: !!! FROM NOW ON CSV MEANS PIPE-SEPARATED-VALUES !!! */
	// 2020-01-28: configurable variable added:
    $csvSep = trim(variable_get('islandora_entities_csv_import_delimiter','|'));
    if ( empty($csvSep) ) { $csvSep = ';'; }
    while (($data = fgetcsv($handle, 0, $csvSep)) !== FALSE) {
      if (!isset($header)) {
        $header = $data;
        continue;
      }
      for ($counter = 0; $counter < count($data); $counter++) {
        $scholar[$header[$counter]] = $data[$counter];
      }
      $scholars[] = $scholar;
    }
    fclose($handle);
  }
  return $scholars;
}

/**
 * Builds scholar objects.
 *
 * @param array $scholar
 *   Header => value pairs representing date to be converted to MADS record
 * @param string $collection_pid
 *   PID of collection to hold new objects.
 * 
 * Will retun the PID of the new/created object, e.g. 'book-authors:1234' if the collection was 'book-authors:collection'
 * 
 * Hint: Typically $scholar is an array (taken from one CSV row) that may look like this:
 *		array(
 *			'GIVEN_NAME'	=> 'Donald',
 *			'FAMILY_NAME'	=> 'Trump',
 *			'DISPLAY_NAME'	=> 'Trump, Donald',
 *			'EMAIL'			=> 'donald.trump@usa.gov',
 *			'DEPARTMENT'	=> 'White House',
 *			'IDENTIFIER'	=> '12345678',
 *		),
 */
function islandora_entities_build_scholar_object($scholar, $collection_pid, $namespace = '', $owner_name = 'admin', $user_id = 0) {
  if ( empty($scholar) || empty($collection_pid) ) {
    return FALSE;
  }
  if ( !( $mads = islandora_entities_build_mads($scholar) ) ) {
    return FALSE;
  }
  if ( empty($owner_name) ) {
	global $user;
	$owner_name = $user->name;
  }
  if ( empty($namespace) ) {
    $namespace = preg_replace('/:collection$/', '', $collection_pid ); /* [DP]: this assumes that collection-pids are of the form 'namespace:collection' */
  }
  $tuque = NULL;
  if ( intval($user_id) < 1 ) {
    $tuque = new IslandoraTuque();
  } elseif( $userTmp = user_load($user_id) ) {
	$tuque = @islandora_get_tuque_connection( $userTmp );
  }
  if ( !$tuque || $tuque == NULL ) {
    return FALSE;
  }
  $repository = $tuque->repository;
  if ( $fedora_object = $repository->constructObject($namespace) ) {
    $fedora_object->label = @empty($scholar['DISPLAY_NAME']) ? 'Member' : $scholar['DISPLAY_NAME'];
    if ( !empty($owner_name) ) {
	  $fedora_object->owner = $owner_name;
    }
    if ( $datastream = $fedora_object->constructDatastream('MADS', 'M') ) {
      $datastream->label = 'MADS';
      $datastream->mimetype = 'application/xml';
      $datastream->content = $mads;
      $fedora_object->ingestDatastream($datastream);
      $fedora_object->relationships->add(FEDORA_MODEL_URI, 'hasModel', 'islandora:personCModel');
      $fedora_object->relationships->add(FEDORA_RELS_EXT_URI, 'isMemberOfCollection', $collection_pid);
      if ( $object_pid = islandora_add_object($fedora_object) ) {
        return $object_pid;
      }
    }
  }
  return FALSE;
}

/**
 * Builds MADS stream from Scholar data.
 *
 * @param array $scholar
 *   Associative array of header => value pairs
 *
 * @return xml
 *   MADS xml representing individual scholar
 */
function islandora_entities_build_mads($scholar) {
  $dom = new DomDocument('1.0', 'UTF-8');
  $dom->preserveWhiteSpace = FALSE;
  $dom->formatOutput = TRUE;
  $mads_uri = "http://www.loc.gov/mads/v2";
  $root = $dom->createElement('mads');
  $dom->appendChild($root);

  $ns_mads = $dom->createAttribute('xmlns');
  $root->appendChild($ns_mads);
  $value = $dom->createTextNode($mads_uri);
  $ns_mads->appendChild($value);

  $ns_mads_qualified = $dom->createAttribute('xmlns:mads');
  $root->appendChild($ns_mads_qualified);
  $value = $dom->createTextNode($mads_uri);
  $ns_mads_qualified->appendChild($value);

  $xsi = $dom->createAttribute('xmlns:xsi');
  $root->appendChild($xsi);
  $value = $dom->createTextNode("http://www.w3.org/2001/XMLSchema-instance");
  $xsi->appendChild($value);

  $xlink = $dom->createAttribute('xmlns:xlink');
  $root->appendChild($xlink);
  $value = $dom->createTextNode("http://www.w3.org/1999/xlink");
  $xlink->appendChild($value);

  islandora_entities_add_authority($dom, $scholar);
  islandora_entities_add_variant($dom, $scholar);
  islandora_entities_add_affiliation($dom, $scholar);
  islandora_entities_add_building_address($dom, $scholar);

  if (isset($scholar['IDENTIFIER'])) {
    $identifier = islandora_entities_attach_element($dom, "identifier", $root, $scholar['IDENTIFIER']);
    $identifier->setAttribute('type', 'u1');
  }
  if (isset($scholar['STATUS'])) {
    $status = islandora_entities_attach_element($dom, 'note', $root, $scholar['STATUS']);
    $status->setAttribute('type', 'status');
  }
  return $dom->saveXML();
}

/**
 * Adds authority.
 *
 * @param DomDocument $dom
 *   Document holding MADS data
 * @param array $scholar
 *   Associative array of header => value pairs
 */
function islandora_entities_add_authority($dom, $scholar) {
  $name = array();
  $nameparts = array(
    'GIVEN_NAME',
    'FAMILY_NAME',
    'TERM_OF_ADDRESS',
    'NAME_DATE',
  );
  foreach ($nameparts as $namepart) {
    if (isset($scholar[$namepart])) {
      $name[$namepart] = $scholar[$namepart];
    }
  }
  if (count($name) == 0) {
    return;
  }
  $root = $dom->documentElement;
  $authority = islandora_entities_attach_element($dom, "authority", $root);
  $name_node = islandora_entities_attach_element($dom, 'name', $authority);
  $name_node->setAttribute('type', 'personal');
  if (isset($name['GIVEN_NAME'])) {
    $name_part = islandora_entities_attach_element($dom, 'namePart', $name_node, $name['GIVEN_NAME']);
    $name_part->setAttribute('type', 'given');
  }
  if (isset($name['FAMILY_NAME'])) {
    $name_part = islandora_entities_attach_element($dom, 'namePart', $name_node, $name['FAMILY_NAME']);
    $name_part->setAttribute('type', 'family');
  }
  if (isset($name['TERM_OF_ADDRESS'])) {
    $name_part = islandora_entities_attach_element($dom, 'namePart', $name_node, $name['TERM_OF_ADDRESS']);
    $name_part->setAttribute('type', 'termsOfAddress');
  }
  if (isset($name['NAME_DATE'])) {
    $name_part = islandora_entities_attach_element($dom, 'namePart', $name_node, $name['NAME_DATE']);
    $name_part->setAttribute('type', 'date');
  }
  if (isset($scholar['DISPLAY_NAME'])) {
    $title_info = islandora_entities_attach_element($dom, "titleInfo", $authority);
    islandora_entities_attach_element($dom, "title", $title_info, $scholar['DISPLAY_NAME']);
  }
}

/**
 * Adds authority.
 *
 * @param DomDocument $dom
 *   Document holding MADS data
 * @param array $scholar
 *   Associative array of header => value pairs
 */
function islandora_entities_add_variant($dom, $scholar) {
  $name = array();
  $nameparts = array(
    'VAR_GIVEN_NAME',
    'VAR_FAMILY_NAME',    
  );
  foreach ($nameparts as $namepart) {
    if (isset($scholar[$namepart])) {
      $name[$namepart] = $scholar[$namepart];
    }
  }
  if (count($name) == 0) {
    return;
  }
  $root = $dom->documentElement;
  $authority = islandora_entities_attach_element($dom, "variant", $root);
  $name_node = islandora_entities_attach_element($dom, 'name', $authority);
  if (isset($name['VAR_GIVEN_NAME'])) {
    $name_part = islandora_entities_attach_element($dom, 'namePart', $name_node, $name['VAR_GIVEN_NAME']);
    $name_part->setAttribute('type', 'given');
  }
  if (isset($name['VAR_FAMILY_NAME'])) {
    $name_part = islandora_entities_attach_element($dom, 'namePart', $name_node, $name['VAR_FAMILY_NAME']);
    $name_part->setAttribute('type', 'family');
  }
}


/**
 * Adds affiliation.
 *
 * @param DomDocument $dom
 *   Document holding MADS data
 * @param array $scholar
 *   Associative array of header => value pairs
 * 
 * 2020-01-28: Revised due to http://lib-dora-dev1.emp-eaw.ch:3000/issues/186
 * XML structure fixed according to http://www.loc.gov/standards/mads/mads-outline-2-0.html
 */
function islandora_entities_add_affiliation($dom, $scholar) {
  $affiliations = array();
  $address = array();	// finally a sub-elment of <affiliation>
  $affiliation_parts = array(
    'POSITION',
    'DEPARTMENT',	/* correspoing element is called <organization> ! */
    'EMAIL',
    'PHONE',
    'FAX',
    'HOURS',
  );
  $date_parts = array(	/* date-related sub-elements inside <affiliation> */ 
    'START_DATE',
    'END_DATE',
  );
  $address_parts = array(	/* inside <address>, which is a sub-elements of <affiliation> */ 
    'STREET',
    'CITY',
    'STATE',
    'COUNTRY',
    'POSTCODE',
  );
  foreach ($affiliation_parts as $affiliation_part) {
    if (@!empty($scholar[$affiliation_part])) {
      $affiliations[$affiliation_part] = $scholar[$affiliation_part];
    }
  }
  if ( count($affiliations) == 0 ) {
    return;
  }
  foreach ($address_parts as $address_part) {
    if (@!empty($scholar[$address_part])) {
      $address[$address_part] = $scholar[$address_part];
    }
  }
  $root = $dom->documentElement;
  $affiliation = islandora_entities_attach_element($dom, "affiliation", $root);
  foreach( $affiliation_parts as $affiliation_part ) {
    if (isset($affiliations[$affiliation_part])) {
	  $nodename = ( $affiliation_part == "DEPARTMENT" ? "organization" : strtolower($affiliation_part) );
      islandora_entities_attach_element($dom, $nodename, $affiliation, $affiliations[$affiliation_part]);
    }
  }
  if (count($address) > 0) {
    $address = islandora_entities_attach_element($dom, "address", $affiliation);
    foreach ($address_parts as $item) {
      if (@!empty($scholar[$item])) {
        $nodename = strtolower($item);
        islandora_entities_attach_element($dom, $nodename, $address, $scholar[$item]);
      }
    }
  }
  foreach ($date_parts as $point) {
    if (@!empty($scholar[$point])) {
      $line = islandora_entities_attach_element($dom, "dateValid", $affiliation, $scholar[$point]);
      $line->setAttribute('point', strtolower(strtok($point,'_')) );
    }
  }
}

/**
 * Adds address as note.
 *
 * @param DomDocument $dom
 *   Document holding MADS data
 * @param array $scholar
 *   Associative array of header => value pairs
 */
function islandora_entities_add_building_address($dom, $scholar) {
  $address_fields = array(
    'ROOM_NUMBER',
    'BUILDING',
    'CAMPUS',
  );
  $address_string = '';
  foreach ($address_fields as $field) {
    if (isset($scholar[$field])) {
      $part = $scholar[$field];
      $address_string .= "$part \n";
    }
  }
  if ($address_string) {
    $root = $dom->documentElement;
    $note = islandora_entities_attach_element($dom, 'note', $root, $address_string);
    $note->setAttribute('type', 'address');
  }
}

/*
 * function to encode special characters in names/strings.
 * Examples:
 *		- Åsgard      -->  &#197;sgard
 *		- Özmir       -->  &#214;sgard
 *		- &Ouml;zmir  -->  &#214;sgard
 * 		- Preuß       -->  Preu&#223;
 * 
 * Note that in an UTF-8 encode file you may not see all of these encodings directly.
 */
function islandora_entities_xml_value_safe($term, $deHtml = true, $deUrl = true) {
	if ( $deUrl ) {
		$term = rawurldecode($term);
	}
	if ( $deHtml ) {
		$term = html_entity_decode($term); // to handle Umlaut-Encoding like '&Ouml;' - mb_encode_numericentity will *numerically* encode again.
	}
	$term = utf8_encode(utf8_decode(trim($term)));
	$ampAry = array();
	preg_match_all('/&\W|&\s|&$/', $term, $ampAry, PREG_OFFSET_CAPTURE);
	foreach( @array_reverse($ampAry[0]) as $aAry ) {	// $aAry: 0: found part, 1: position
		if ( $aAry[0] != "&#" ) {	// rough check, however intended to find+skip numerically encoded chars like '&#214;'
			$term = substr($term,0,$aAry[1]) . '&amp;' . substr($term,$aAry[1]+1);
		}
	}
	while( ( $pos = stripos($term,"&amp;amp;") ) !== false ) {	// rather for safety, should not be necessary
		$term = substr($term,0,$pos+1) . substr($term,$pos+5);
	}
	$encAry = array(
		'<' => '&lt;',
		'>' => '&gt;',
		'"' => '&quot;',
		"'" => '&apos;',
		urldecode('%C3%9F') => '&#223;',
		urldecode('%E1%BA%9E') => '&#223;',
	);
	foreach( $encAry as $chr => $enc ) {
		$term = str_replace($chr,$enc,$term);
	}
	return mb_encode_numericentity($term, array(0x80,0xff,0,0xff), 'UTF-8');
}

/**
 * Creates, populates and attaches new node to Dom object.
 *
 * @param DOMDocument $dom
 *   MADS DOMDocument
 * @param string $element_name
 *   Name of element to be attached
 * @param DOMElement $parent_node
 *   Node to which new element will be attached
 * @param string $content
 *   Optional content for created node
 *
 * @return DOMElement
 *   Created node
 */
function islandora_entities_attach_element($dom, $element_name, $parent_node, $content = NULL, $safe_value = TRUE) {
  if ( $content == "" || $content == NULL ) {
    $node = $dom->createElement($element_name);
    $parent_node->appendChild($node);
  }
  elseif ( $safe_value ) {		// since 2020-01-28:
    $elements = explode('~', str_replace('||', ',', $content));		// the || handing quite obsolete, left as legacy support
    foreach ($elements as $element_content) {
      $node = $dom->createElement($element_name, islandora_entities_xml_value_safe($element_content) );
      $parent_node->appendChild($node);
    }
  }
  else { // original/former code
    $elements = explode('~', htmlspecialchars(str_replace('||', ',', $content)));
    foreach ($elements as $element_content) {
      $node = $dom->createElement($element_name, trim($element_content) );
      $parent_node->appendChild($node);
    }
  }
  return $node;
}

/**
 * Autocomplete for collection selection.
 *
 * @param string $string
 *   Contents of textfield being completed
 */
function islandora_entities_get_collection_autocomplete($string = '') {
  $list = array();
  if ($string) {
    $string = str_replace(':', '\:', $string);
    $query = "RELS_EXT_hasModel_uri_ms:\"info:fedora/islandora:collectionCModel\" AND (dc.title:{$string}* OR PID:{$string}*)";
    $qp = new IslandoraSolrQueryProcessor();
    $qp->buildQuery($query);
    $qp->solrParams['fl'] = 'dc.title, PID';
    $qp->executeQuery(FALSE);
    try {
      $results = $qp->islandoraSolrResult['response']['objects'];
    }
    catch (Exception $e) {
      watchdog_exception('Islandora Entities', $e, 'Got an exception while searching entities for callback.', array(), WATCHDOG_ERROR);
      $results = array();
    }
    if ($results) {
      foreach ($results as $choice) {
        $list[$choice['PID']] = $choice['solr_doc']['dc.title'][0] . " ~ " . $choice['PID'];
      }
    }
  }

  drupal_json_output($list);
}

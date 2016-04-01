<?php

function icg_csv_load_csv_data($csvfile, $delimiter=",") {
  $fh = fopen($csvfile, "r");
  $contentsA = array();
 
  while (($data = fgetcsv($fh,0,$delimiter)) !== FALSE) {
     array_push($contentsA, $data);
  }
  fclose($fh);
  return $contentsA;
} // end FUNCTION icg_csv_load_csv_data

function icg_remove_special_chars($val) {
	$val = htmlspecialchars($val);
}

function icg_csv_parse_csv_data($csvA) {

// make sure that we escape XML special characters: <>&'"
array_walk_recursive($csvA, htmlspecialchars); //'icg_remove_special_chars');

//ASSUMPTION: first row in array is headers
  $headers = 0;
  $headersA = array();
  $contentsA = array();

  foreach($csvA as $csv) {
      if ($headers == 0) {
	$headersA = $csv;
	$headers++;	
      }
      else {
        array_push($contentsA, $csv);
      }
  } // end WHILE csv is open

// TODO we need to further delimit by semicolon or some other delimiter for multiple values in fields

  // now, we have two arrays: one with the first line, and the other with the rest of the delimited data
  /*
	[0] => array(heading stuff)
	[1] => array(data)
	
	WANT: array([0] => array(headers);
		    [1] => array(row1);
		    [2] => array(row2);
		etc.
		)

   */
	array_unshift($contentsA, $headersA);
	return $contentsA;
} // end FUNCTION icg_csv_parse_csv_data

?>

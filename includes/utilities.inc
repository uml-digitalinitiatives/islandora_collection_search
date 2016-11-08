<?php
/**
 * @file
 * Utility functions for the collection search module.
 */

/**
 * Retrieves all searchable collections.
 *
 * @param array $options
 *   The default set of options to be appended to.
 * @param bool|string $collection
 *   Bool denoting whether we are on a collection when searching, the PID
 *   otherwise.
 *
 * @return array
 *   An associative array where the key is the pid of the collection object
 *   and the value is the label.
 */
function islandora_collection_search_retrieve_searchable_collections($options, $collection = FALSE) {
  $searchable_collections = variable_get('islandora_collection_search_searchable_collections', array());
  $searchable_collections = array_filter($searchable_collections);
  foreach ($searchable_collections as $collection_pid) {
    // Don't want to add the current collection twice or inaccessible
    // collections.
    $search_object = islandora_object_load($collection_pid);
    if ($search_object && islandora_object_access(ISLANDORA_VIEW_OBJECTS, $search_object) && $collection_pid != $collection) {
      $options[$collection_pid] = $search_object->label;
    }
  }
  return $options;
}

/**
 * Gets a list of all parent pids.
 *
 * @param AbstractObject $object
 *   The object.
 *
 * @return array
 *   The list of parent PIDs.
 */
function islandora_collection_search_get_parent_pids(AbstractObject $object) {
  $results = array_merge(
    $object->relationships->get(FEDORA_RELS_EXT_URI, 'isMemberOfCollection'),
    $object->relationships->get(FEDORA_RELS_EXT_URI, 'isMemberOf'),
    $object->relationships->get(FEDORA_RELS_EXT_URI, 'isConstituentOf')
  );
  $map_results = function($o) {
    return $o['object']['value'];
  };
  $collections = array_map($map_results, $results);
  return array_unique(array_filter($collections));
}

/**
 * Search up the tree for a valid object to search.
 *
 * @param string $pid
 *   The PID of the object.
 * @param array $list
 *   The pid => label list.
 * @param boolean $last
 *   Whether this is the last parent.
 *
 * @return string
 *   The PID of the valid collection.
 */
function islandora_collection_search_get_parents($pid, $list = array(), $last = false) {
  $object = islandora_object_load($pid);
  if ($object) {
    if (in_array('islandora:newspaperissuecmodel', array_map('strtolower', $object->models))) {
      $list = array_merge(array($pid => "This issue"), $list);
    }
    else if (in_array('islandora:newspapercmodel', array_map('strtolower', $object->models))) {
      $list = array_merge(array($pid => "This newspaper"), $list);
      $last = true;
    }
    else if (in_array('islandora:collectioncmodel', array_map('strtolower', $object->models))) {
      $list = array_merge(array($pid => "This collection"), $list);
      $last = true;
    }

    if (!$last) {
      $parents = islandora_collection_search_get_parent_pids($object);
      if (count($parents) == 1 || count($list) > 0) {
        $parent_pid = reset($parents);
        $list = array_merge(islandora_collection_search_get_parents($parent_pid, $list), $list);
      }
      else if (count($parents) > 1) {
        foreach ($parents as $parent) {
          $list = array_merge(islandora_collection_search_get_parents($parent, $list), $list);
        }
      }
    }
  }
  return $list;
}

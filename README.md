# drush-module-cleanup

## Installation

* Put the *.inc file into your $HOME/.drush/ folder
* Run drush cc drush

That's it!

## Usage

```
drush system-table-check
```
Gives you a list of entries in the system table for modules that are no longer present in your site. 

**After you reviewed these** , run
```
drush system-table-clean
```

This will clean the system table. 
Clear caches and check the website.

## Note:
D7 only!

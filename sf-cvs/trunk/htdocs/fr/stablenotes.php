<?php BoxTop("$releaseNotesStr $stableVersion"); ?>

<pre>
Probl�mes connus:

* Windows: La palette flottante ne change pas de format quand on r�duit la fen�tre principale. Pour contourner ce probl�me, gardez la palette d'outils attach�e � la fen�tre de projet.

* Linux: le duplex int�gral (lecture d'une piste et enregistrement d'une autre en simultan�)
  ne fonctionne pas m�me si la carte de son est con�ue pour le faire. Ce probl�me est probablement d� � notre fa�on d'utiliser OSS.

* Linux: avec la version actuelle, les cartes de son qui ne traitent que plus de 2 canaux ne sont pas support�es.

-------------------------------------------------------------

Modifications dans la version 1.0:

  * Mac OS:

    - Audacity ne quitte plus lorsqu'on ferme la derni�re fen�tre. La barre de menus demeure et on peut ouvrir une nouvelle fen�tre.

    - Correction d'un bogue qui emp�chait la fen�tre de Pr�f�rences de s'afficher sur OS 9 lorsque le tableau de bord ATM �tait actif.

    - Correction d'un probl�me qui emp�chait l'enregistrement � des taux d'�chantillonnage faibles avec Mac OS 9
      
    - Reconnaissance des fichiers MP3 par l'extension et par le code type.

    - Audacity est maintenant plus stable et plus fonctionnel sous MacOS X

  * Nouveaut�s:

    - Ajout d'une bo�te de dialogue "Sauvegarder les changements" sur toutes les plateformes.

    - Bulles d'aide

    - Nouveaux raccourcis clavier

    - Les pistes de rep�rage peuvent maintenant �tre utilis�es pour baliser des r�gions s�lectionn�es. Il y a une nouvelle commande pour ajouter des rep�res -- voir le guide d'utilisation.

  * Divers:

    - Diverses corrections de bogues

Modifications dans la version 0.98b (version de correction pour Windows seulement):

  - Corrrection du plantage d� � l'effet "Elimination du bruit"

  - Ajout d'une bo�te de dialogue "Sauvegarder les changements"

  - Retour du support des rep�res ID3 sur Windows

  - Diverses corrections de bogues

Modifications dans la version 0.98:

* Effets

  - Nouvel effet d'inversion
  - Nouvel effet R�trograde
  - Am�lioration de l'effet �limination de bruit

* Corrections de bogues

  - Il est maintenant possible de copier-coller entre projets sans risques.

  - Correction de bogues lors du chargement, de la sauvegarde et de l'annulation d'enveloppes.

  - Il est maintenant impossible d'ouvrir le m�me projet dans deux fen�tres diff�rentes, ce qui aurait entra�n� la perte des donn�es avec les versions ant�rieures.

  - Les projets sont maintenant sauvegard�s en tenant compte de la position de chaque piste.

  - Meilleure gestion des cas o� le fichier temporaire est invalide au lancement du programme

  - Ne r��crit plus un fichier dont le projet d�pendait (il renomme l'ancien fichier).  Il est maintenant possible d'exporter vers un fichier qui porte le m�me que le fichier import�.

  - Enregistrer sous...  pour un projet n'efface plus l'ancienne version.

  - Les informations pour les annulations sont d�truites � la fermeture du projet pour r�cup�rer l'espace du disque dur qui �tait ainsi perdu dans les versions pr�c�dentes.

  - Enregistrer sous... ne plante plus lorsqu'un mauvais emplacement est choisi.

  - La copie d'une piste avec un d�calage fonctionne.

  - Correction d'un autre plantage dans l'effet Amplification lorsque rien n'est s�lectionn� dans une des pistes.

  - Windows: les dispositifs d'enregistrement et de lecture  ne sont plus invers�s dans les pr�f�rences!

  - Windows: les plugiciels VSTsont d�tect�s peu importe comment Audacity est lanc�.

  - Windows: ajout de l'importation de fichiers Ogg Vorbis.

  - Il ne se cr�e plus de piste fant�me lorsque l'enregistrement �choue.

  - La s�lection d'un format d'exportation diff�rent se r�fl�te dans la barre de menus.

  - Diverses corrections de bogues mineurs

* Unix:

  - Nouveau code OSS devrait am�liorer la lecture et l'enregistrement sur presque tous les syst�mes Unix OSS.

  - Preliminary support for the KDE/aRts soundserver (a compile-time option; doit �tre configur� pour �tre utilis� � la place du code OSS)

  - Corrections de bogues dans le script "make install" 

Modifications dans la version 0.97:

* Installation/configuration

  - Nouvel installateur Windows

  - Plus d'options dans le script de configuration sur Unix

* Interface utilisateur:

  - Correction du bogue du menu Effets qui gelait lorsqu'une s�lection n'incluait pas la totalit� des pistes.

  - Ajout de l'effet Elimination du bruit (Dominic)

  - Improved click-drag zoom.

  - Support du glisser-d�poser pour importer un fichier audio (Windows)

  - Am�lioration de l'exportation de fichier (une bo�te de dialogue s'affiche lorsque l'extension du nom de fichier est inconnue)

  - Diverses corrections

* MacOS:

  - Correction du bogue de d�filement automatique qui arrivait parfois lorsque le curseur �tait rel�ch� hors de la fen�tre lors d'une s�lection. 

* Unix:

  - Correction du bogue de l'effet Amplification qui faisait parfois geler � l'ouverture de la bo�te de dialogue.

Modifications dans la version 0.96:

* Interface utilisateur:

  - Ajout de boutons mute/solo

* Importation Audio:

  - Fixed regression bug in 0.95 which caused stereo files to be imported as
    two mono tracks
    
  - Importation des rep�res ID3 dans les fichiers MP3

* Exportation Audio:

  - Exportation en MP3 fonctionne maintenant � condition d'avoir la version ad�quate de LAME DLL (Joshua)
  
  - L'�dition des rep�res ID3 des fichiers MP3 � l'exportation est maintenant possible.

* Pr�f�rences:

  - Added Audio I/O selectors on Mac (Dominic) and Windows (Joshua)

* Effets:

  - Ajout d'un indicateur d'�tat � l'application de tous les effets qui permet �galement d'en annuler l'application.
  
  - Added support for stereo effects and effects that add or
    remove tracks, or require multiple passes.
  
  - Am�lioration de l'effet Amplifier et correction de tous les bogues connus (Dominic)
  
  - Effet Bass Boost am�lior�
  
  - Ajout de l'effet Filtre (Dominic)
  
  - Ajout de l'effect Phaser (Paul)
  
  - Ajout del'effect Wahwah  (Paul)

Changes in 0.95:

* Installation/Compilation:

  - Improved configure script on unix systems (Joshua)

*  Interface utilisateur:

  - Menu items are now disabled when unavailable

  - Online help added (Dominic and Logan)

* Importing Audio:

  - Lazy import added, speeding up importing of PCM files by 2x

  - Added support for the Free libmpeg3 library on unix 
    to replace the proprietary xaudio (Joshua)

  - Importing MP3 and Ogg Vorbis files is now handled automatically
    by the Open and Import commands.

  - Fixed the Import Raw Data feature, so now you can
    import files of almost any arbitrary format (as long
    as it's uncompressed).

* Fen�tre principale:

  - New track labels with a single integrated pop-up menu
    to handle all track options

  - Vertical ruler added, along with preliminary support for
    vertical zooming

  - Stereo tracks can be linked together so changes affect
    both tracks

  - Point-sample display takes over when you zoom very far in

  - Two new wave displays: a dB (logarithmic) wave display and
    a spectral pitch display (using enhanced autocorrelation)

* Pr�f�rences:

  - New spectral display preferences

  - Temp directory can be set in preferences

* Frequency display:

  - Many new frequency window enhancements, including support for
    cepstrum, autocorrelation, and enhanced autocorrelation.

* Editeur d'enveloppe:

  - Envelopes are now interpolated using decibels, making
    cross-fades sound much better

* Effets:

  - Fixed a bug that caused incompatibility with many VST plug-ins.

  - Added Maximize Amplitude effect

  - Ajout de l'effet Bass Boost (Paul)

* Other:

  - Improved memory management over long Undo histories

  - Diverses corrections de bogues

Changes in 0.94:

* Preferences dialog (Joshua Haberman)

* OGG Vorbis import (Joshua Haberman)

* Silence, Insert Silence commands

* Split and Duplicate commands

* Mac OS X support

* Supports recording on Mac OS 8 and 9

* Diverses corrections

Changes in 0.93:

* Affichage de l'indicateur de position pendant l'enregistrement et la lecture.

* Keeps track of some preferences

* Supports arbitrary project sample rate

* Mac: ouverture de documents � partir du Finder

* Floating tool palette is now dockable
  (and docked by default)

* Fixed bugs in handling multiple open projects

* Supports recording (Windows, Linux)

* Frequency Window displays note names (i.e. C4, G#5)

* Many bug fixes for effects, including VST plug-in effects

Changes in 0.92:

* Added Frequency Plot window and improved Spectrum display

* Fixed bug in File:Open when the file to be opened was
  actually a large WAV file

Changes in 0.91:

* Utilisation de la biblioth�que xaudio pour l'importation de fichiers MP3.

* Menu Zoom

Changes in 0.9:

* New floating tool palette with four tools (selection,
  sliding, zooming, and envelope editing) plus play and
  stop buttons

* Playback now mixes tracks, and you can work with the
  document while listening.  The stop button works.

* Rewritten file handling functions.  The main view
  is no longer dependent on the wxWindows DocView
  classes, so we can handle files ourselves.  The
  project file format is now text-based for easy
  debugging.  Eventually it will probably move to XML.

* Improved handling of wave tracks: as before, the data
  is stored in blocks, but now, the blocks are correctly
  limited to betweek n and 2n bytes each (for some n),
  which guarantees editing operations always take the
  same amount of time, while also ensuring that projects
  don't get more fragmented over time.

* Rewritten user interface code.  The shades of gray
  are taken from the OS, and the project window has been
  redesigned to have more consistent layout across all
  platforms.

* Selecting "Open" now does the smart thing, opening a
  project if you give it a project, or importing a WAV
  file if you give it that.

* Flashing cursor indicates the current editing position

* R�gle grandement am�lior�e - en plus d'avoir meilleure apparence, elle affiche maintenant la s�lection et le curseur.

* The zoom tool centers on the cursor so you can zoom
  into wherever you are.

</pre>

<?php BoxBottom(); ?>

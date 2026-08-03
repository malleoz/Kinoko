//////////////////////////////////////////////////////////////////////
// Fix inherited member descriptions that don't collapse (Doxygen bug) //
//////////////////////////////////////////////////////////////////////
// Doxygen omits the 'inherit <group>' classes from a memdesc row whenever
// that member also gets a "More..." link (i.e. it has its own detailed
// documentation section, which happens for every documented enum). Since
// dynsections.js's toggleInherit() only shows/hides rows matching
// 'tr.inherit.<group>', and the default stylesheet only hides '.inherit'
// rows, an unclassed memdesc row is always visible, even when its
// "inherited from" section is collapsed. Recover the missing classes from
// the memdesc row's memitem sibling, which is always classed correctly.
document.querySelectorAll('tr[class^="memdesc"]').forEach(memdesc => {
    if (memdesc.classList.contains('inherit')) {
        return;
    }

    const memitem = memdesc.previousElementSibling;
    if (!memitem || !memitem.classList.contains('inherit')) {
        return;
    }

    memitem.classList.forEach(cls => {
        if (!cls.startsWith('memitem:')) {
            memdesc.classList.add(cls);
        }
    });
});

////////////////////////////
// Remove "More..." Links //
////////////////////////////

document.querySelectorAll('a').forEach(link => {
    if (link.textContent.trim() === 'More...') {
        link.remove();
    }
});

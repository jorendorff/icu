/*
*******************************************************************************
*
*   Copyright (C) 1996-2014, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*
* CollationLoader.java, ported from ucol_res.cpp
*/

package com.ibm.icu.impl.coll;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.util.MissingResourceException;

import com.ibm.icu.impl.ICUResourceBundle;
import com.ibm.icu.util.Output;
import com.ibm.icu.util.ULocale;
import com.ibm.icu.util.UResourceBundle;

/**
 * Convenience string denoting the Collation data tree
 */
public final class CollationLoader {

    // not implemented, all methods are static
    private CollationLoader() {
    }

    private static volatile String rootRules = null;

    private static void loadRootRules() {
        if (rootRules != null) {
            return;
        }
        synchronized(CollationLoader.class) {
            if (rootRules == null) {
                UResourceBundle rootBundle = UResourceBundle.getBundleInstance(
                        ICUResourceBundle.ICU_COLLATION_BASE_NAME, ULocale.ROOT);
                rootRules = rootBundle.getString("UCARules");
            }
        }
    }

    // C++: static void appendRootRules(UnicodeString &s)
    public static String getRootRules() {
        loadRootRules();
        return rootRules;
    }

    static String loadRules(ULocale locale, CharSequence collationType) {
        UResourceBundle bundle = UResourceBundle.getBundleInstance(
                ICUResourceBundle.ICU_COLLATION_BASE_NAME, locale);
        UResourceBundle data = ((ICUResourceBundle)bundle).getWithFallback("collations/" + collationType);
        String rules = data.getString("Sequence");
        return rules;
    }

    public static CollationTailoring loadTailoring(ULocale locale, Output<ULocale> outValidLocale) {

        // Java porting note: ICU4J getWithFallback/getStringWithFallback currently does not
        // work well when alias table is involved in a resource path, unless full path is specified.
        // For now, collation resources does not contain such data, so the code below should work fine.

        CollationTailoring root = CollationRoot.getRoot();

        UResourceBundle bundle = null;
        try {
            bundle = UResourceBundle.getBundleInstance(
                    ICUResourceBundle.ICU_COLLATION_BASE_NAME, locale);
        } catch (MissingResourceException e) {
            if (outValidLocale != null) {
                outValidLocale.value = ULocale.ROOT;
            }
            return root;
        }

        ULocale validLocale = bundle.getULocale();
        if (outValidLocale != null) {
            outValidLocale.value = validLocale;
        }

        // There are zero or more tailorings in the collations table.
        UResourceBundle collations = ((ICUResourceBundle)bundle).get("collations");
        if (collations == null) {
            return root;
        }

        // Fetch the collation type from the locale ID and the default type from the data.
        String type = locale.getKeywordValue("collation");
        String defaultType = "standard";

        String defT = ((ICUResourceBundle)collations).getStringWithFallback("default");
        if (defT != null) {
            defaultType = defT;
        }

        if (type == null) {
            type = defaultType;
        }

        // Load the collations/type tailoring, with type fallback.

        // Java porting note: typeFallback is used for setting U_USING_DEFAULT_WARNING in
        // ICU4C, but not used by ICU4J

        // boolean typeFallback = false;
        UResourceBundle data = ((ICUResourceBundle)collations).getWithFallback(type);
        if (data == null &&
                type.length() > 6 && type.regionMatches(true, 0, "search", 0, 6)) {
            // fall back from something like "searchjl" to "search"
            // typeFallback = true;
            type = type.substring(0, 6);
            data = ((ICUResourceBundle)collations).getWithFallback(type);
        }

        if (data == null && !type.equals(defaultType)) {
            // fall back to the default type
            // typeFallback = true;
            type = defaultType;
            data = ((ICUResourceBundle)collations).getWithFallback(type);
        }

        if (data == null) {
            return root;
        }

        // Is this the same as the root collator? If so, then use that instead.
        ULocale actualLocale = data.getULocale();
        if (actualLocale.equals(ULocale.ROOT) && type.equals("standard")) {
            return root;
        }

        CollationTailoring t = new CollationTailoring(root.settings);
        t.actualLocale = actualLocale;

        // deserialize
        UResourceBundle binary = ((ICUResourceBundle)data).get("%%CollationBin");
        byte[] inBytes = binary.getBinary(null);
        ByteArrayInputStream inStream = new ByteArrayInputStream(inBytes);
        try {
            CollationDataReader.read(root, inStream, t);
        } catch (IOException e) {
            throw new RuntimeException("Failed to load collation tailoring data for locale:"
                    + actualLocale + " type:" + type, e);
        }   // No need to close BAIS.

        // Try to fetch the optional rules string.
        String seq = ((ICUResourceBundle)data).getString("Sequence");
        if (seq != null) {
            t.rules = seq;
        }

        // Set the collation types on the informational locales,
        // except when they match the default types (for brevity and backwards compatibility).
        // For the valid locale, suppress the default type.
        if (outValidLocale != null && !type.equals(defaultType)) {
            outValidLocale.value = validLocale.setKeywordValue("collation", type);
        }

        // For the actual locale, suppress the default type *according to the actual locale*.
        // For example, zh has default=pinyin and contains all of the Chinese tailorings.
        // zh_Hant has default=stroke but has no other data.
        // For the valid locale "zh_Hant" we need to suppress stroke.
        // For the actual locale "zh" we need to suppress pinyin instead.
        if (!actualLocale.equals(validLocale)) {
            // Opening a bundle for the actual locale should always succeed.
            UResourceBundle actualBundle = UResourceBundle.getBundleInstance(
                    ICUResourceBundle.ICU_COLLATION_BASE_NAME, actualLocale);
            defT = ((ICUResourceBundle)actualBundle).getStringWithFallback("collations/default");
            defaultType = (defT == null) ? "standard" : defT;
        }

        if (!type.equals(defaultType)) {
            t.actualLocale = t.actualLocale.setKeywordValue("collation", type);
        }

        // if (typeFallback) {
        //     ICU4C implementation sets U_USING_DEFAULT_WARNING here
        // }

        return t;
    }
}
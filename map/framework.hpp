#pragma once

#include "events.hpp"
#include "drawer_yg.hpp"
#include "tile_renderer.hpp"
#include "information_display.hpp"
#include "window_handle.hpp"
#include "location_state.hpp"
#include "navigator.hpp"
#include "feature_vec_model.hpp"
#include "bookmark.hpp"

#include "../defines.hpp"

#include "../search/search_engine.hpp"

#include "../storage/storage.hpp"

#include "../indexer/mercator.hpp"
#include "../indexer/data_header.hpp"
#include "../indexer/scales.hpp"

#include "../platform/platform.hpp"
#include "../platform/location.hpp"

#include "../yg/defines.hpp"
#include "../yg/screen.hpp"
#include "../yg/color.hpp"
#include "../yg/render_state.hpp"
#include "../yg/skin.hpp"
#include "../yg/resource_manager.hpp"
#include "../yg/info_layer.hpp"

#include "../coding/file_reader.hpp"
#include "../coding/file_writer.hpp"

#include "../geometry/rect2d.hpp"
#include "../geometry/screenbase.hpp"

#include "../base/logging.hpp"
//#include "../base/mutex.hpp"
#include "../base/timer.hpp"

#include "../std/vector.hpp"
#include "../std/shared_ptr.hpp"
#include "../std/scoped_ptr.hpp"
#include "../std/target_os.hpp"


//#define DRAW_TOUCH_POINTS

class DrawerYG;
class RenderPolicy;
namespace search { class Result; }

class Framework
{
protected:
  mutable scoped_ptr<search::Engine> m_pSearchEngine;
  model::FeaturesFetcher m_model;
  Navigator m_navigator;

  vector<BookmarkCategory *> m_bookmarks;

  scoped_ptr<RenderPolicy> m_renderPolicy;
  bool m_hasPendingInvalidate, m_doForceUpdate, m_queryMaxScaleMode, m_drawPlacemark;

  m2::AnyRectD m_invalidRect;
  m2::PointD m_placemark;

  InformationDisplay m_informationDisplay;

  double const m_metresMinWidth;
  double const m_metresMaxWidth;
  int const m_minRulerWidth;

  int m_width;
  int m_height;

  enum TGpsCenteringMode
  {
    EDoNothing,
    ECenterAndScale,
    ECenterOnly
  };

  TGpsCenteringMode m_centeringMode;
  location::State m_locationState;

  //mutable threads::Mutex m_modelSyn;

  storage::Storage m_storage;

  //my::Timer m_timer;
  inline double ElapsedSeconds() const
  {
    //return m_timer.ElapsedSeconds();
    return 0.0;
  }

  /// Stores lowest loaded map version
  /// Holds -1 if no maps were added
  /// @see feature::DataHeader::Version
  int m_lowestMapVersion;

  void AddMap(string const & file);
  void RemoveMap(string const & datFile);
  /// Only file names
  void GetLocalMaps(vector<string> & outMaps);

  void DrawAdditionalInfo(shared_ptr<PaintEvent> const & e);

public:
  Framework();
  virtual ~Framework();

  /// @name Used on iPhone for upgrade from April 1.0.1 version
  //@{
  /// @return true if client should display delete old maps dialog before using downloader
  bool NeedToDeleteOldMaps() const;
  void DeleteOldMaps();
  //@}

  void AddLocalMaps();
  void RemoveLocalMaps();

  void AddBookmark(string const & category, Bookmark const & bm);
  inline size_t GetBmCategoriesCount() const { return m_bookmarks.size(); }
  BookmarkCategory * GetBmCategory(size_t index) const;

  /// Find or create new category by name.
  BookmarkCategory * GetBmCategory(string const & name);

  /// Get bookmark by touch.
  /// @param[in]  pixPt   Coordinates of touch point in pixels.
  /// @return     NULL    If not biikmark near the point.
  Bookmark const * GetBookmark(m2::PointD pixPt) const;

  void ClearBookmarks();

  inline m2::PointD PtoG(m2::PointD const & p) const { return m_navigator.PtoG(p); }
  inline m2::PointD GtoP(m2::PointD const & p) const { return m_navigator.GtoP(p); }

  storage::Storage & Storage() { return m_storage; }

  void OnLocationStatusChanged(location::TLocationStatus newStatus);
  void OnGpsUpdate(location::GpsInfo const & info);
  void OnCompassUpdate(location::CompassInfo const & info);

  void SetRenderPolicy(RenderPolicy * renderPolicy);
  RenderPolicy * GetRenderPolicy() const;

  InformationDisplay & GetInformationDisplay();

  void PrepareToShutdown();

  void SetupMeasurementSystem();

  RenderPolicy::TRenderFn DrawModelFn();

  void DrawModel(shared_ptr<PaintEvent> const & e,
                 ScreenBase const & screen,
                 m2::RectD const & selectRect,
                 m2::RectD const & clipRect,
                 int scaleLevel,
                 bool isTiling);

private:
  inline m2::RectD GetCurrentViewport() const { return m_navigator.Screen().ClipRect(); }
  search::Engine * GetSearchEngine() const;

  void CheckMinGlobalRect(m2::RectD & r) const;

public:
  void PrepareSearch(bool hasPt, double lat = 0.0, double lon = 0.0);
  void Search(search::SearchParams const & params);
  bool GetCurrentPosition(double & lat, double & lon) const;
  void ShowSearchResult(search::Result const & res);

  string GetCountryName(m2::PointD const & pt) const;

  /// @return country code in ISO 3166-1 alpha-2 format (two small letters) or empty string
  string GetCountryCodeByPosition(double lat, double lon) const;

  void SetMaxWorldRect();

  void Invalidate(bool doForceUpdate = false);
  void InvalidateRect(m2::RectD const & rect, bool doForceUpdate = false);

  void SaveState();
  bool LoadState();

  /// Resize event from window.
  virtual void OnSize(int w, int h);

  bool SetUpdatesEnabled(bool doEnable);

  //double GetCurrentScale() const;
  int GetDrawScale() const;

  m2::PointD GetViewportCenter() const;
  void SetViewportCenter(m2::PointD const & pt);

  bool SetViewportByURL(string const & url);

  bool NeedRedraw() const;
  void SetNeedRedraw(bool flag);

  inline void XorQueryMaxScaleMode()
  {
    m_queryMaxScaleMode = !m_queryMaxScaleMode;
    Invalidate(true);
  }

  /// Get classificator types for nearest features.
  /// @param[in] pixPt Current touch point in device pixel coordinates.
  void GetFeatureTypes(m2::PointD pixPt, vector<string> & types) const;

  struct AddressInfo
  {
    string m_country, m_city, m_street, m_house, m_name;
  };

  /// Get address information for point on map.
  /// @param[in] pt Point in mercator coordinates.
  void GetAddressInfo(m2::PointD const & pt, AddressInfo & info) const;

  virtual void BeginPaint(shared_ptr<PaintEvent> const & e);
  /// Function for calling from platform dependent-paint function.
  virtual void DoPaint(shared_ptr<PaintEvent> const & e);

  virtual void EndPaint(shared_ptr<PaintEvent> const & e);

  void ShowRect(m2::RectD rect);
  void ShowRectFixed(m2::RectD rect);

  void DrawPlacemark(m2::PointD const & pt);
  void DisablePlacemark();

  void MemoryWarning();
  void EnterBackground();
  void EnterForeground();

  /// @TODO refactor to accept point and min visible length
  //void CenterAndScaleViewport();

  /// Show all model by it's world rect.
  void ShowAll();

  /// @name Drag implementation.
  //@{
  void StartDrag(DragEvent const & e);
  void DoDrag(DragEvent const & e);
  void StopDrag(DragEvent const & e);
  void Move(double azDir, double factor);
  //@}

  /// @name Rotation implementation
  //@{
  void StartRotate(RotateEvent const & e);
  void DoRotate(RotateEvent const & e);
  void StopRotate(RotateEvent const & e);
  //@}

  /// @name Scaling.
  //@{
  void ScaleToPoint(ScaleToPointEvent const & e);
  void ScaleDefault(bool enlarge);
  void Scale(double scale);
  void StartScale(ScaleEvent const & e);
  void DoScale(ScaleEvent const & e);
  void StopScale(ScaleEvent const & e);
  //@}

private:
  //bool IsEmptyModel() const;
  bool IsEmptyModel(m2::PointD const & pt);
};
